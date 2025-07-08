#ifndef SOCKETMANAGER_HPP
#define SOCKETMANAGER_HPP

#include <string>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>

class SocketManager {
private:
    bool parseIPAddress(const std::string& host, struct sockaddr_in& addr);
    bool setSocketOptions(int sock_fd);
    bool setNonBlocking(int sock_fd);

public:
    SocketManager();
    ~SocketManager();
    
    int createListenSocket(const std::string& host, int port);
    void closeSocket(int sock_fd);
    static std::string ipToString(const struct sockaddr_in& addr);
};

#endif