#include "SocketManager.hpp"
#include <iostream>
#include <sstream>
#include <cstdio>
#include <algorithm>

/*
 * Default constructor for SocketManager
 * Initializes the socket manager with default state
 */
SocketManager::SocketManager() {}

/*
 * Destructor for SocketManager
 * Cleans up any allocated resources
 */
SocketManager::~SocketManager() {}

/*
 * Parses an IP address string into a sockaddr_in structure
 * Handles special cases like 0.0.0.0 and validates IP format
 * Returns true if parsing succeeds, false otherwise
 */
bool SocketManager::parseIPAddress(const std::string& host, struct sockaddr_in& addr) {
    if (host == "0.0.0.0" || host.empty()) {
        addr.sin_addr.s_addr = INADDR_ANY;
        return true;
    }
    
    // Parse IP address manually using C++ string streams
    std::stringstream ss(host);
    std::string token;
    unsigned int ip_parts[4];
    int count = 0;
    
    while (std::getline(ss, token, '.') && count < 4) {
        std::stringstream token_ss(token);
        token_ss >> ip_parts[count];
        if (token_ss.fail() || !token_ss.eof() || ip_parts[count] > 255) {
            std::cerr << "Invalid IP address format: " << host << std::endl;
            return false;
        }
        count++;
    }
    
    if (count != 4) {
        std::cerr << "Invalid IP address format: " << host << std::endl;
        return false;
    }
    
    addr.sin_addr.s_addr = htonl((ip_parts[0] << 24) | (ip_parts[1] << 16) | 
                                (ip_parts[2] << 8) | ip_parts[3]);
    return true;
}

/*
 * Sets socket options for the listening socket
 * Enables SO_REUSEADDR to allow immediate socket reuse
 * Returns true if options are set successfully, false otherwise
 */
bool SocketManager::setSocketOptions(int sock_fd) {
    int opt = 1;
    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Error setting SO_REUSEADDR" << std::endl;
        return false;
    }
    return true;
}

/*
 * Sets a socket to non-blocking mode
 * Required for proper functioning with poll/select
 * Returns true if successful, false otherwise
 */
bool SocketManager::setNonBlocking(int sock_fd) {
    int flags = fcntl(sock_fd, F_SETFL, O_NONBLOCK);
    if (flags < 0 || fcntl(sock_fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        std::cerr << "Error setting non-blocking" << std::endl;
        return false;
    }
    return true;
}

/*
 * Creates a listening socket bound to the specified host and port
 * Sets up the socket with proper options (reuse address, non-blocking)
 * Returns the socket file descriptor on success, -1 on error
 */
int SocketManager::createListenSocket(const std::string& host, int port) {
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        std::cerr << "Error creating socket" << std::endl;
        return -1;
    }
    
    if (!setSocketOptions(sock_fd)) {
        close(sock_fd);
        return -1;
    }
    
    if (!setNonBlocking(sock_fd)) {
        close(sock_fd);
        return -1;
    }
    
    struct sockaddr_in addr;
    std::fill(reinterpret_cast<char*>(&addr), reinterpret_cast<char*>(&addr) + sizeof(addr), 0);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    
    if (!parseIPAddress(host, addr)) {
        close(sock_fd);
        return -1;
    }
    
    if (bind(sock_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Error binding to " << host << ":" << port << std::endl;
        close(sock_fd);
        return -1;
    }
    
    if (listen(sock_fd, 128) < 0) {
        std::cerr << "Error listening on socket" << std::endl;
        close(sock_fd);
        return -1;
    }
    
    std::cout << "Listening on " << host << ":" << port << std::endl;
    return sock_fd;
}

/*
 * Closes a socket file descriptor
 * Wrapper function for the close() system call
 */
void SocketManager::closeSocket(int sock_fd) {
    close(sock_fd);
}

/*
 * Converts a sockaddr_in IP address to a human-readable string
 * Extracts individual bytes and formats them as dotted decimal notation
 * Returns the IP address as a string (e.g., "192.168.1.1")
 */
std::string SocketManager::ipToString(const struct sockaddr_in& addr) {
    unsigned char* ip_bytes = (unsigned char*)&addr.sin_addr.s_addr;
    std::stringstream ss;
    ss << (int)ip_bytes[0] << "." << (int)ip_bytes[1] << "." 
       << (int)ip_bytes[2] << "." << (int)ip_bytes[3];
    return ss.str();
}