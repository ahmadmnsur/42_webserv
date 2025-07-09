#include "ConnectionHandler.hpp"
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <errno.h>
#include <cstring>

/*
 * Default constructor for ConnectionHandler
 * Initializes the connection handler with empty client map
 */
ConnectionHandler::ConnectionHandler() {}

/*
 * Destructor for ConnectionHandler
 * Closes all client connections and cleans up resources
 */
ConnectionHandler::~ConnectionHandler() {
    closeAllClients();
}

/*
 * Closes all client connections and clears the client map
 * Used for graceful shutdown
 */
void ConnectionHandler::closeAllClients() {
    
    for (std::map<int, ClientData>::iterator it = _clients.begin(); 
         it != _clients.end(); ++it) {
        _socket_manager.closeSocket(it->first);
    }
    _clients.clear();
}

/*
 * Accepts a new client connection on a listening socket
 * Sets the client socket to non-blocking mode and creates ClientData
 * Returns the client socket file descriptor, or -1 on error
 */
int ConnectionHandler::acceptNewConnection(int listen_sock) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    int client_sock = accept(listen_sock, (struct sockaddr*)&client_addr, &client_len);
    if (client_sock < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            std::cerr << "Error accepting connection: " << strerror(errno) << std::endl;
        }
        return -1;
    }
    
    int flags = fcntl(client_sock, F_GETFL, 0);
    if (flags < 0 || fcntl(client_sock, F_SETFL, flags | O_NONBLOCK) < 0) {
        std::cerr << "Error setting client socket non-blocking: " << strerror(errno) << std::endl;
        _socket_manager.closeSocket(client_sock);
        return -1;
    }
    
    _clients[client_sock] = ClientData();
    
    std::string client_ip = SocketManager::ipToString(client_addr);
    std::cout << "New connection from " << client_ip << ":" << ntohs(client_addr.sin_port) 
              << " (fd: " << client_sock << ")" << std::endl;
    
    return client_sock;
}

/*
 * Processes incoming data from a client
 * Appends data to the client's read buffer and creates HTTP response
 * Currently implements echo functionality for testing
 */
void ConnectionHandler::processClientData(int client_sock, const char* buffer, ssize_t bytes_read) {
    _clients[client_sock].appendToReadBuffer(buffer, bytes_read);
    
    std::cout << "Received " << bytes_read << " bytes from client " << client_sock << std::endl;
    std::cout << "Data: " << std::string(buffer, bytes_read) << std::endl;
    
    std::string response = createHttpResponse(std::string(buffer, bytes_read));
    _clients[client_sock].setWriteBuffer(response);
    _clients[client_sock].setBytesSent(0);
}

/*
 * Creates a basic HTTP response with the given content
 * Includes proper HTTP headers and content length
 * Returns a complete HTTP response string
 */
std::string ConnectionHandler::createHttpResponse(const std::string& content) {
    std::string response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: text/plain\r\n";
    
    std::stringstream ss;
    ss << content.size();
    response += "Content-Length: " + ss.str() + "\r\n";
    response += "Connection: close\r\n";
    response += "\r\n";
    response += content;
    
    return response;
}

/*
 * Handles reading data from a client socket
 * Receives data, processes it, or handles connection errors/disconnects
 * Uses non-blocking I/O and handles EAGAIN/EWOULDBLOCK appropriately
 */
void ConnectionHandler::handleClientRead(int client_sock) {
    char buffer[4096];
    ssize_t bytes_read = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
    
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        processClientData(client_sock, buffer, bytes_read);
    } else if (bytes_read == 0) {
        std::cout << "Client " << client_sock << " disconnected" << std::endl;
        removeClient(client_sock);
    } else {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            std::cerr << "Error reading from client " << client_sock 
                      << ": " << strerror(errno) << std::endl;
            removeClient(client_sock);
        }
    }
}

/*
 * Handles writing data to a client socket
 * Sends pending response data and tracks bytes sent
 * Removes client when response is fully sent or on error
 */
void ConnectionHandler::handleClientWrite(int client_sock) {
    ClientData& client = _clients[client_sock];
    
    if (client.getBytesSent() >= client.getWriteBuffer().size()) {
        return;
    }
    
    ssize_t bytes_sent = send(client_sock, 
                             client.getWriteBuffer().c_str() + client.getBytesSent(),
                             client.getWriteBuffer().size() - client.getBytesSent(), 0);
    
    if (bytes_sent > 0) {
        client.setBytesSent(client.getBytesSent() + bytes_sent);
        std::cout << "Sent " << bytes_sent << " bytes to client " << client_sock << std::endl;
        
        if (client.getBytesSent() >= client.getWriteBuffer().size()) {
            std::cout << "Finished sending response to client " << client_sock << std::endl;
            removeClient(client_sock);
        }
    } else if (bytes_sent == 0) {
        std::cout << "Client " << client_sock << " closed connection during write" << std::endl;
        removeClient(client_sock);
    } else {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            std::cerr << "Error writing to client " << client_sock 
                      << ": " << strerror(errno) << std::endl;
            removeClient(client_sock);
        }
    }
}

/*
 * Removes a client from the connection handler
 * Closes the socket and erases client data from the map
 */
void ConnectionHandler::removeClient(int client_sock) {
    _clients.erase(client_sock);
    _socket_manager.closeSocket(client_sock);
    std::cout << "Removed client " << client_sock << std::endl;
}

/*
 * Checks if a client socket exists in the handler
 * Returns true if client is managed by this handler
 */
bool ConnectionHandler::hasClient(int client_sock) const {
    return _clients.find(client_sock) != _clients.end();
}

/*
 * Returns a reference to the ClientData for the given socket
 * Non-const version for modifying client data
 */
ClientData& ConnectionHandler::getClient(int client_sock) {
    return _clients[client_sock];
}

/*
 * Returns a const reference to the ClientData for the given socket
 * Const version for read-only access to client data
 */
const ClientData& ConnectionHandler::getClient(int client_sock) const {
    std::map<int, ClientData>::const_iterator it = _clients.find(client_sock);
    return it->second;
}