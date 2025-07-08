#ifndef WEBSERVER_HPP
#define WEBSERVER_HPP

#include "ServerConfig.hpp"
#include "ConnectionHandler.hpp"
#include "SocketManager.hpp"
#include <poll.h>
#include <vector>

class WebServer {
private:
    std::vector<ServerConfig> _configs;
    std::vector<int> _listen_sockets;
    std::vector<pollfd> _poll_fds;
    ConnectionHandler _connection_handler;
    SocketManager _socket_manager;
    
    void setupSockets();
    void handleNewConnection(int listen_sock);
    void updatePollEvents(int client_sock, short events);
    bool isListenSocket(int fd) const;
    
public:
    WebServer(const std::vector<ServerConfig>& server_configs);
    ~WebServer();
    void run();
    bool isValid() const;
};

#endif