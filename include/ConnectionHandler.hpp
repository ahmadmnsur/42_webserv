#ifndef CONNECTIONHANDLER_HPP
#define CONNECTIONHANDLER_HPP

#include "ClientData.hpp"
#include "SocketManager.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "ServerConfig.hpp"
#include <map>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>

class ConnectionHandler {
private:
    std::map<int, ClientData> _clients;
    SocketManager _socket_manager;
    const std::vector<ServerConfig>* _server_configs;
    
    HttpResponse processHttpRequest(const HttpRequest& request);
    void processClientData(int client_sock, const char* buffer, ssize_t bytes_read);
    const Location* findMatchingLocation(const std::string& uri) const;
    std::string sanitizePath(const std::string& path) const;
    std::string getMimeType(const std::string& path) const;
    const ServerConfig* getCurrentServerConfig(int client_sock) const;
    HttpResponse executeCgiScript(const std::string& script_path, const std::string& interpreter_path, 
                                  const HttpRequest& request, const std::string& file_path) const;
    HttpResponse handleFileUpload(const HttpRequest& request, const Location* location, const std::string& uri);
    HttpResponse createErrorResponse(int error_code) const;
    std::string urlDecode(const std::string& encoded) const;

public:
    ConnectionHandler();
    ~ConnectionHandler();
    
    void setServerConfigs(const std::vector<ServerConfig>& configs);
    
    int acceptNewConnection(int listen_sock);
    void handleClientRead(int client_sock);
    void handleClientWrite(int client_sock);
    void removeClient(int client_sock);
    void closeAllClients(); // New method for cleanup
    std::vector<int> checkEmptyRequestTimeouts(); // Check for clients with empty/incomplete request timeouts, returns clients needing POLLOUT
    
    bool hasClient(int client_sock) const;
    ClientData& getClient(int client_sock);
    const ClientData& getClient(int client_sock) const;
};

#endif