#include "WebServer.hpp"
#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <errno.h>
#include <cstring>

/*
 * Constructor for WebServer
 */
WebServer::WebServer(const std::vector<ServerConfig>& server_configs, const SignalManager& signal_manager) 
    : _configs(server_configs), _signal_manager(signal_manager) {
    _connection_handler.setServerConfigs(_configs);
    setupSockets();
}

/*
 * Destructor for WebServer
 */
WebServer::~WebServer() {
    cleanup();
}

/*
 * Performs complete cleanup of all server resources
 */
void WebServer::cleanup() {
    // Close all listening sockets
    for (size_t i = 0; i < _listen_sockets.size(); ++i) {
        _socket_manager.closeSocket(_listen_sockets[i]);
    }
    _listen_sockets.clear();
    
    // Clear poll file descriptors
    _poll_fds.clear();
}

/*
 * Checks if the server is valid
 */
bool WebServer::isValid() const {
    return !_listen_sockets.empty();
}

/*
 * Checks if the given file descriptor is a listening socket
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
 */
void WebServer::run() {
    std::cout << "Server running with " << _listen_sockets.size() << " listening sockets" << std::endl;

    while (!_signal_manager.isShutdownRequested()) {
        int poll_count = poll(&_poll_fds[0], _poll_fds.size(), 1000); // 1 second timeout
        
        if (poll_count < 0) {
            // Poll error - do not check errno as per 42 requirements
            std::cerr << "Poll error occurred" << std::endl;
            break;
        }
        
        if (poll_count == 0) {
            // Timeout, check for empty request timeouts
            std::vector<int> clients_needing_pollout = _connection_handler.checkEmptyRequestTimeouts();
            for (size_t i = 0; i < clients_needing_pollout.size(); ++i) {
                updatePollEvents(clients_needing_pollout[i], POLLIN | POLLOUT);
            }
            continue; // Check shutdown flag
        }
        
        // Always check for empty request timeouts on each iteration
        std::vector<int> clients_needing_pollout = _connection_handler.checkEmptyRequestTimeouts();
        for (size_t i = 0; i < clients_needing_pollout.size(); ++i) {
            updatePollEvents(clients_needing_pollout[i], POLLIN | POLLOUT);
        }
        
        // Check all file descriptors
        for (size_t i = 0; i < _poll_fds.size() && !_signal_manager.isShutdownRequested(); ++i) {
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
}