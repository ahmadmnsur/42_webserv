#include "WebServer.hpp"
#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <errno.h>
#include <cstring>

/*
 * Constructor for WebServer
 * Initializes the server with provided configurations and sets up listening sockets
 */
WebServer::WebServer(const std::vector<ServerConfig>& server_configs) 
    : _configs(server_configs) {
    setupSockets();
}

/*
 * Destructor for WebServer
 * Properly closes all listening sockets and cleans up resources
 */
WebServer::~WebServer() {
    cleanup();
}

/*
 * Performs complete cleanup of all server resources
 * Closes all sockets and cleans up connections
 */
void WebServer::cleanup() {
    std::cout << "Cleaning up server resources..." << std::endl;
    
    // Close all listening sockets
    for (size_t i = 0; i < _listen_sockets.size(); ++i) {
        std::cout << "Closing listening socket " << _listen_sockets[i] << std::endl;
        _socket_manager.closeSocket(_listen_sockets[i]);
    }
    _listen_sockets.clear();
    
    // Close all client connections through ConnectionHandler destructor
    // The ConnectionHandler destructor will handle closing client sockets
    
    // Clear poll file descriptors
    _poll_fds.clear();
    
    std::cout << "Cleanup complete." << std::endl;
}

/*
 * Checks if the server is valid (has at least one listening socket)
 * Returns true if valid, false otherwise
 */
bool WebServer::isValid() const {
    return !_listen_sockets.empty();
}

/*
 * Checks if the given file descriptor is a listening socket
 * Returns true if fd is a listening socket, false otherwise
 */
bool WebServer::isListenSocket(int fd) const {
    for (size_t i = 0; i < _listen_sockets.size(); ++i) {
        if (fd == _listen_sockets[i]) {
            return true;
        }
    }
    return false;
}

/*
 * Sets up listening sockets for each server configuration
 * Creates sockets, adds them to the poll fd list for monitoring
 * Validates that at least one socket was created successfully
 */
void WebServer::setupSockets() {
    for (size_t i = 0; i < _configs.size(); ++i) {
        int sock_fd = _socket_manager.createListenSocket(_configs[i].getHost(), _configs[i].getPort());
        if (sock_fd >= 0) {
            _listen_sockets.push_back(sock_fd);
            
            pollfd pfd;
            pfd.fd = sock_fd;
            pfd.events = POLLIN;
            pfd.revents = 0;
            _poll_fds.push_back(pfd);
        }
    }
    
    if (_listen_sockets.empty()) {
        std::cerr << "No valid listening sockets created!" << std::endl;
        return;
    }
}

/*
 * Handles a new client connection on a listening socket
 * Accepts the connection and adds the client socket to the poll fd list
 * Sets up the client socket for read monitoring
 */
void WebServer::handleNewConnection(int listen_sock) {
    int client_sock = _connection_handler.acceptNewConnection(listen_sock);
    if (client_sock >= 0) {
        pollfd pfd;
        pfd.fd = client_sock;
        pfd.events = POLLIN;
        pfd.revents = 0;
        _poll_fds.push_back(pfd);
    }
}

/*
 * Updates the poll events for a specific client socket
 * Modifies the events mask (POLLIN, POLLOUT) for the socket
 * Used to switch between read and write monitoring modes
 */
void WebServer::updatePollEvents(int client_sock, short events) {
    for (size_t i = 0; i < _poll_fds.size(); ++i) {
        if (_poll_fds[i].fd == client_sock) {
            _poll_fds[i].events = events;
            break;
        }
    }
}

/*
 * Main server loop
 * Uses poll() to monitor sockets and handles incoming connections and data
 * Continues until shutdown is requested via signal handler
 */
void WebServer::run() {
    std::cout << "Server running with " << _listen_sockets.size() << " listening sockets" << std::endl;
    std::cout << "Press Ctrl+C to shutdown gracefully..." << std::endl;
    
    while (!g_shutdown_requested) {
        // Use short timeout in poll to check shutdown flag periodically
        int poll_count = poll(&_poll_fds[0], _poll_fds.size(), 1000); // 1 second timeout
        
        if (poll_count < 0) {
            if (errno == EINTR) {
                // Interrupted by signal, check shutdown flag
                std::cout << "\nSignal received - initiating graceful shutdown..." << std::endl;
                break;
            }
            std::cerr << "Poll error: " << strerror(errno) << std::endl;
            break;
        }
        
        if (poll_count == 0) {
            continue; // Timeout, check shutdown flag
        }
        
        // Check all file descriptors
        for (size_t i = 0; i < _poll_fds.size() && !g_shutdown_requested; ++i) {
            if (_poll_fds[i].revents == 0) {
                continue;
            }
            
            int fd = _poll_fds[i].fd;
            
            if (isListenSocket(fd)) {
                if (_poll_fds[i].revents & POLLIN) {
                    handleNewConnection(fd);
                }
            } else {
                if (_poll_fds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
                    std::cout << "Client " << fd << " error/hangup" << std::endl;
                    _connection_handler.removeClient(fd);
                    for (std::vector<pollfd>::iterator it = _poll_fds.begin(); it != _poll_fds.end(); ++it) {
                        if (it->fd == fd) {
                            _poll_fds.erase(it);
                            break;
                        }
                    }
                    --i;
                } else {
                    if (_poll_fds[i].revents & POLLIN) {
                        _connection_handler.handleClientRead(fd);
                        if (_connection_handler.hasClient(fd)) {
                            updatePollEvents(fd, POLLIN | POLLOUT);
                        }
                    }
                    if (_poll_fds[i].revents & POLLOUT) {
                        _connection_handler.handleClientWrite(fd);
                        if (!_connection_handler.hasClient(fd)) {
                            for (std::vector<pollfd>::iterator it = _poll_fds.begin(); it != _poll_fds.end(); ++it) {
                                if (it->fd == fd) {
                                    _poll_fds.erase(it);
                                    break;
                                }
                            }
                            --i;
                        }
                    }
                }
            }
        }
    }
    
    std::cout << "Server main loop exited. Performing cleanup..." << std::endl;
}

// ===== Error handling wrapper functions for robustness =====

/* 
 * Safe memory allocation wrapper
 * Returns NULL on failure instead of throwing
 */
template<typename T>
T* safe_new(size_t count = 1) {
    try {
        return new T[count];
    } catch (const std::bad_alloc&) {
        return NULL;
    }
}

/*
 * Safe delete wrapper
 * Checks for NULL pointer before deletion
 */
template<typename T>
void safe_delete(T*& ptr) {
    if (ptr) {
        delete ptr;
        ptr = NULL;
    }
}

/*
 * Safe array delete wrapper
 * Checks for NULL pointer before deletion
 */
template<typename T>
void safe_delete_array(T*& ptr) {
    if (ptr) {
        delete[] ptr;
        ptr = NULL;
    }
}