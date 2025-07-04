// #include "webserv.hpp"
// #include <sstream> //
// #include <cstdlib> // For exit()



// // Implementation
// #include <iostream>

// WebServer::WebServer(const std::vector<ServerConfig>& server_configs) 
//     : configs(server_configs) {
//     setupSockets();
// }

// WebServer::~WebServer() {
//     // Close all sockets
//     for (size_t i = 0; i < listen_sockets.size(); ++i) {
//         close(listen_sockets[i]);
//     }
    
//     for (std::map<int, ClientData>::iterator it = clients.begin(); 
//          it != clients.end(); ++it) {
//         close(it->first);
//     }
// }

// int WebServer::createListenSocket(const std::string& host, int port) {
//     int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
//     if (sock_fd < 0) {
//         std::cerr << "Error creating socket: " << strerror(errno) << std::endl;
//         return -1;
//     }
    
//     // Set SO_REUSEADDR
//     int opt = 1;
//     if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
//         std::cerr << "Error setting SO_REUSEADDR: " << strerror(errno) << std::endl;
//         close(sock_fd);
//         return -1;
//     }
    
//     // Set non-blocking
//     int flags = fcntl(sock_fd, F_GETFL, 0);
//     if (flags < 0 || fcntl(sock_fd, F_SETFL, flags | O_NONBLOCK) < 0) {
//         std::cerr << "Error setting non-blocking: " << strerror(errno) << std::endl;
//         close(sock_fd);
//         return -1;
//     }
    
//     // Bind
//     struct sockaddr_in addr;
//     std::memset(&addr, 0, sizeof(addr));
//     addr.sin_family = AF_INET;
//     addr.sin_port = htons(port);
    
//     if (host == "0.0.0.0" || host.empty()) {
//         addr.sin_addr.s_addr = INADDR_ANY;
//     } else {
//         if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) <= 0) {
//             std::cerr << "Invalid host address: " << host << std::endl;
//             close(sock_fd);
//             return -1;
//         }
//     }
    
//     if (bind(sock_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
//         std::cerr << "Error binding to " << host << ":" << port 
//                   << " - " << strerror(errno) << std::endl;
//         close(sock_fd);
//         return -1;
//     }
    
//     // Listen
//     if (listen(sock_fd, 128) < 0) {
//         std::cerr << "Error listening on socket: " << strerror(errno) << std::endl;
//         close(sock_fd);
//         return -1;
//     }
    
//     std::cout << "Listening on " << host << ":" << port << std::endl;
//     return sock_fd;
// }

// void WebServer::setupSockets() {
//     for (size_t i = 0; i < configs.size(); ++i) {
//         int sock_fd = createListenSocket(configs[i].host, configs[i].port);
//         if (sock_fd >= 0) {
//             listen_sockets.push_back(sock_fd);
            
//             pollfd pfd;
//             pfd.fd = sock_fd;
//             pfd.events = POLLIN;
//             pfd.revents = 0;
//             poll_fds.push_back(pfd);
//         }
//     }
    
//     if (listen_sockets.empty()) {
//         std::cerr << "No valid listening sockets created!" << std::endl;
//         exit(1);
//     }
// }

// void WebServer::handleNewConnection(int listen_sock) {
//     struct sockaddr_in client_addr;
//     socklen_t client_len = sizeof(client_addr);
    
//     int client_sock = accept(listen_sock, (struct sockaddr*)&client_addr, &client_len);
//     if (client_sock < 0) {
//         if (errno != EAGAIN && errno != EWOULDBLOCK) {
//             std::cerr << "Error accepting connection: " << strerror(errno) << std::endl;
//         }
//         return;
//     }
    
//     // Set client socket non-blocking
//     int flags = fcntl(client_sock, F_GETFL, 0);
//     if (flags < 0 || fcntl(client_sock, F_SETFL, flags | O_NONBLOCK) < 0) {
//         std::cerr << "Error setting client socket non-blocking: " << strerror(errno) << std::endl;
//         close(client_sock);
//         return;
//     }
    
//     // Add to poll list
//     pollfd pfd;
//     pfd.fd = client_sock;
//     pfd.events = POLLIN;
//     pfd.revents = 0;
//     poll_fds.push_back(pfd);
    
//     // Initialize client data
//     clients[client_sock] = ClientData();
    
//     char client_ip[INET_ADDRSTRLEN];
//     inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
//     std::cout << "New connection from " << client_ip << ":" << ntohs(client_addr.sin_port) 
//               << " (fd: " << client_sock << ")" << std::endl;
// }

// void WebServer::handleClientRead(int client_sock) {
//     char buffer[4096];
//     ssize_t bytes_read = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
    
//     if (bytes_read > 0) {
//         buffer[bytes_read] = '\0';
//         clients[client_sock].read_buffer.append(buffer, bytes_read);
        
//         std::cout << "Received " << bytes_read << " bytes from client " << client_sock << std::endl;
//         std::cout << "Data: " << std::string(buffer, bytes_read) << std::endl;
        
//         // Echo server: prepare response
//         std::string response = "HTTP/1.1 200 OK\r\n";
//         response += "Content-Type: text/plain\r\n";
//         std::ostringstream oss;
//         oss << bytes_read;
//         response += "Content-Length: " + oss.str() + "\r\n";
//         response += "Connection: close\r\n";
//         response += "\r\n";
//         response += std::string(buffer, bytes_read);
        
//         clients[client_sock].write_buffer = response;
//         clients[client_sock].bytes_sent = 0;
        
//         // Enable POLLOUT for this socket
//         for (size_t i = 0; i < poll_fds.size(); ++i) {
//             if (poll_fds[i].fd == client_sock) {
//                 poll_fds[i].events = POLLIN | POLLOUT;
//                 break;
//             }
//         }
//     } else if (bytes_read == 0) {
//         std::cout << "Client " << client_sock << " disconnected" << std::endl;
//         removeClient(client_sock);
//     } else {
//         if (errno != EAGAIN && errno != EWOULDBLOCK) {
//             std::cerr << "Error reading from client " << client_sock 
//                       << ": " << strerror(errno) << std::endl;
//             removeClient(client_sock);
//         }
//     }
// }

// void WebServer::handleClientWrite(int client_sock) {
//     ClientData& client = clients[client_sock];
    
//     if (client.bytes_sent >= client.write_buffer.size()) {
//         return; // Nothing to send
//     }
    
//     ssize_t bytes_sent = send(client_sock, 
//                              client.write_buffer.c_str() + client.bytes_sent,
//                              client.write_buffer.size() - client.bytes_sent, 0);
    
//     if (bytes_sent > 0) {
//         client.bytes_sent += bytes_sent;
//         std::cout << "Sent " << bytes_sent << " bytes to client " << client_sock << std::endl;
        
//         if (client.bytes_sent >= client.write_buffer.size()) {
//             std::cout << "Finished sending response to client " << client_sock << std::endl;
//             removeClient(client_sock); // Close connection after response
//         }
//     } else if (bytes_sent == 0) {
//         std::cout << "Client " << client_sock << " closed connection during write" << std::endl;
//         removeClient(client_sock);
//     } else {
//         if (errno != EAGAIN && errno != EWOULDBLOCK) {
//             std::cerr << "Error writing to client " << client_sock 
//                       << ": " << strerror(errno) << std::endl;
//             removeClient(client_sock);
//         }
//     }
// }

// void WebServer::removeClient(int client_sock) {
//     // Remove from poll_fds
//     for (std::vector<pollfd>::iterator it = poll_fds.begin(); it != poll_fds.end(); ++it) {
//         if (it->fd == client_sock) {
//             poll_fds.erase(it);
//             break;
//         }
//     }
    
//     // Remove from clients map
//     clients.erase(client_sock);
    
//     // Close socket
//     close(client_sock);
    
//     std::cout << "Removed client " << client_sock << std::endl;
// }

// void WebServer::run() {
//     std::cout << "Server running with " << listen_sockets.size() << " listening sockets" << std::endl;
    
//     while (true) {
//         int poll_count = poll(&poll_fds[0], poll_fds.size(), -1);
        
//         if (poll_count < 0) {
//             std::cerr << "Poll error: " << strerror(errno) << std::endl;
//             break;
//         }
        
//         if (poll_count == 0) {
//             continue; // Timeout (shouldn't happen with -1)
//         }
        
//         // Check all file descriptors
//         for (size_t i = 0; i < poll_fds.size(); ++i) {
//             if (poll_fds[i].revents == 0) {
//                 continue;
//             }
            
//             int fd = poll_fds[i].fd;
            
//             // Check if it's a listening socket
//             bool is_listen_socket = false;
//             for (size_t j = 0; j < listen_sockets.size(); ++j) {
//                 if (fd == listen_sockets[j]) {
//                     is_listen_socket = true;
//                     break;
//                 }
//             }
            
//             if (is_listen_socket) {
//                 if (poll_fds[i].revents & POLLIN) {
//                     handleNewConnection(fd);
//                 }
//             } else {
//                 // Client socket
//                 if (poll_fds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
//                     std::cout << "Client " << fd << " error/hangup" << std::endl;
//                     removeClient(fd);
//                     --i; // Adjust index since we removed an element
//                 } else {
//                     if (poll_fds[i].revents & POLLIN) {
//                         handleClientRead(fd);
//                     }
//                     if (poll_fds[i].revents & POLLOUT) {
//                         handleClientWrite(fd);
//                     }
//                 }
//             }
//         }
//     }
// }


#include "WebServer.hpp"
#include <iostream>
#include <cstdlib>
#include <sstream>

WebServer::WebServer(const std::vector<ServerConfig>& server_configs) 
    : _configs(server_configs) {
    setupSockets();
}

WebServer::~WebServer() {
    // Close all sockets
    for (size_t i = 0; i < _listen_sockets.size(); ++i) {
        close(_listen_sockets[i]);
    }
    
    for (std::map<int, ClientData>::iterator it = _clients.begin(); 
         it != _clients.end(); ++it) {
        close(it->first);
    }
}

int WebServer::createListenSocket(const std::string& host, int port) {
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        std::cerr << "Error creating socket: " << strerror(errno) << std::endl;
        return -1;
    }
    
    // Set SO_REUSEADDR
    int opt = 1;
    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Error setting SO_REUSEADDR: " << strerror(errno) << std::endl;
        close(sock_fd);
        return -1;
    }
    
    // Set non-blocking
    int flags = fcntl(sock_fd, F_GETFL, 0);
    if (flags < 0 || fcntl(sock_fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        std::cerr << "Error setting non-blocking: " << strerror(errno) << std::endl;
        close(sock_fd);
        return -1;
    }
    
    // Bind
    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    
    if (host == "0.0.0.0" || host.empty()) {
        addr.sin_addr.s_addr = INADDR_ANY;
    } else {
        if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) <= 0) {
            std::cerr << "Invalid host address: " << host << std::endl;
            close(sock_fd);
            return -1;
        }
    }
    
    if (bind(sock_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Error binding to " << host << ":" << port 
                  << " - " << strerror(errno) << std::endl;
        close(sock_fd);
        return -1;
    }
    
    // Listen
    if (listen(sock_fd, 128) < 0) {
        std::cerr << "Error listening on socket: " << strerror(errno) << std::endl;
        close(sock_fd);
        return -1;
    }
    
    std::cout << "Listening on " << host << ":" << port << std::endl;
    return sock_fd;
}

void WebServer::setupSockets() {
    for (size_t i = 0; i < _configs.size(); ++i) {
        int sock_fd = createListenSocket(_configs[i].getHost(), _configs[i].getPort());
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
        exit(1);
    }
}

void WebServer::handleNewConnection(int listen_sock) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    int client_sock = accept(listen_sock, (struct sockaddr*)&client_addr, &client_len);
    if (client_sock < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            std::cerr << "Error accepting connection: " << strerror(errno) << std::endl;
        }
        return;
    }
    
    // Set client socket non-blocking
    int flags = fcntl(client_sock, F_GETFL, 0);
    if (flags < 0 || fcntl(client_sock, F_SETFL, flags | O_NONBLOCK) < 0) {
        std::cerr << "Error setting client socket non-blocking: " << strerror(errno) << std::endl;
        close(client_sock);
        return;
    }
    
    // Add to poll list
    pollfd pfd;
    pfd.fd = client_sock;
    pfd.events = POLLIN;
    pfd.revents = 0;
    _poll_fds.push_back(pfd);
    
    // Initialize client data
    _clients[client_sock] = ClientData();
    
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
    std::cout << "New connection from " << client_ip << ":" << ntohs(client_addr.sin_port) 
              << " (fd: " << client_sock << ")" << std::endl;
}

void WebServer::handleClientRead(int client_sock) {
    char buffer[4096];
    ssize_t bytes_read = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
    
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        _clients[client_sock].appendToReadBuffer(buffer, bytes_read);
        
        std::cout << "Received " << bytes_read << " bytes from client " << client_sock << std::endl;
        std::cout << "Data: " << std::string(buffer, bytes_read) << std::endl;
        
        // Echo server: prepare response
        std::string response = "HTTP/1.1 200 OK\r\n";
        response += "Content-Type: text/plain\r\n";
        
        // Convert bytes_read to string (C++98 compatible)
        std::stringstream ss;
        ss << bytes_read;
        response += "Content-Length: " + ss.str() + "\r\n";
        response += "Connection: close\r\n";
        response += "\r\n";
        response += std::string(buffer, bytes_read);
        
        _clients[client_sock].setWriteBuffer(response);
        _clients[client_sock].setBytesSent(0);
        
        // Enable POLLOUT for this socket
        for (size_t i = 0; i < _poll_fds.size(); ++i) {
            if (_poll_fds[i].fd == client_sock) {
                _poll_fds[i].events = POLLIN | POLLOUT;
                break;
            }
        }
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

void WebServer::handleClientWrite(int client_sock) {
    ClientData& client = _clients[client_sock];
    
    if (client.getBytesSent() >= client.getWriteBuffer().size()) {
        return; // Nothing to send
    }
    
    ssize_t bytes_sent = send(client_sock, 
                             client.getWriteBuffer().c_str() + client.getBytesSent(),
                             client.getWriteBuffer().size() - client.getBytesSent(), 0);
    
    if (bytes_sent > 0) {
        client.setBytesSent(client.getBytesSent() + bytes_sent);
        std::cout << "Sent " << bytes_sent << " bytes to client " << client_sock << std::endl;
        
        if (client.getBytesSent() >= client.getWriteBuffer().size()) {
            std::cout << "Finished sending response to client " << client_sock << std::endl;
            removeClient(client_sock); // Close connection after response
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

void WebServer::removeClient(int client_sock) {
    // Remove from poll_fds
    for (std::vector<pollfd>::iterator it = _poll_fds.begin(); it != _poll_fds.end(); ++it) {
        if (it->fd == client_sock) {
            _poll_fds.erase(it);
            break;
        }
    }
    
    // Remove from clients map
    _clients.erase(client_sock);
    
    // Close socket
    close(client_sock);
    
    std::cout << "Removed client " << client_sock << std::endl;
}

void WebServer::run() {
    std::cout << "Server running with " << _listen_sockets.size() << " listening sockets" << std::endl;
    
    while (true) {
        int poll_count = poll(&_poll_fds[0], _poll_fds.size(), -1);
        
        if (poll_count < 0) {
            std::cerr << "Poll error: " << strerror(errno) << std::endl;
            break;
        }
        
        if (poll_count == 0) {
            continue; // Timeout (shouldn't happen with -1)
        }
        
        // Check all file descriptors
        for (size_t i = 0; i < _poll_fds.size(); ++i) {
            if (_poll_fds[i].revents == 0) {
                continue;
            }
            
            int fd = _poll_fds[i].fd;
            
            // Check if it's a listening socket
            bool is_listen_socket = false;
            for (size_t j = 0; j < _listen_sockets.size(); ++j) {
                if (fd == _listen_sockets[j]) {
                    is_listen_socket = true;
                    break;
                }
            }
            
            if (is_listen_socket) {
                if (_poll_fds[i].revents & POLLIN) {
                    handleNewConnection(fd);
                }
            } else {
                // Client socket
                if (_poll_fds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
                    std::cout << "Client " << fd << " error/hangup" << std::endl;
                    removeClient(fd);
                    --i; // Adjust index since we removed an element
                } else {
                    if (_poll_fds[i].revents & POLLIN) {
                        handleClientRead(fd);
                    }
                    if (_poll_fds[i].revents & POLLOUT) {
                        handleClientWrite(fd);
                    }
                }
            }
        }
    }
}