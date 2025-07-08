#ifndef CONNECTIONHANDLER_HPP
#define CONNECTIONHANDLER_HPP

#include "ClientData.hpp"
#include "SocketManager.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <map>
#include <string>

class ConnectionHandler {
private:
    std::map<int, ClientData> _clients;
    SocketManager _socket_manager;
    
    void processClientData(int client_sock, const char* buffer, ssize_t bytes_read);
    std::string createHttpResponse(const std::string& content);

public:
    ConnectionHandler();
    ~ConnectionHandler();
    
    int acceptNewConnection(int listen_sock);
    void handleClientRead(int client_sock);
    void handleClientWrite(int client_sock);
    void removeClient(int client_sock);
    bool hasClient(int client_sock) const;
    
    ClientData& getClient(int client_sock);
    const ClientData& getClient(int client_sock) const;
};

#endif