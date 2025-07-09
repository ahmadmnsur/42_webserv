#ifndef CONNECTIONHANDLER_HPP
#define CONNECTIONHANDLER_HPP

#include "ClientData.hpp"
#include "SocketManager.hpp"
#include <map>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>

class ConnectionHandler {
private:
    std::map<int, ClientData> _clients;
    SocketManager _socket_manager;
    
    std::string createHttpResponse(const std::string& content);
    void processClientData(int client_sock, const char* buffer, ssize_t bytes_read);

public:
    ConnectionHandler();
    ~ConnectionHandler();
    
    int acceptNewConnection(int listen_sock);
    void handleClientRead(int client_sock);
    void handleClientWrite(int client_sock);
    void removeClient(int client_sock);
    void closeAllClients(); // New method for cleanup
    
    bool hasClient(int client_sock) const;
    ClientData& getClient(int client_sock);
    const ClientData& getClient(int client_sock) const;
};

#endif