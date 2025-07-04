
#ifndef WEBSERVER_HPP
#define WEBSERVER_HPP

#include "ServerConfig.hpp"
#include "ClientData.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>
#include <map>
#include <vector>

class WebServer {
private:
    std::vector<ServerConfig> _configs;
    std::vector<int> _listen_sockets;
    std::vector<pollfd> _poll_fds;
    std::map<int, ClientData> _clients;
    
    int createListenSocket(const std::string& host, int port);
    void setupSockets();
    void handleNewConnection(int listen_sock);
    void handleClientRead(int client_sock);
    void handleClientWrite(int client_sock);
    void removeClient(int client_sock);
    
public:
    WebServer(const std::vector<ServerConfig>& server_configs);
    ~WebServer();
    void run();
};

#endif