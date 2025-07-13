#ifndef SERVERCONFIG_HPP
#define SERVERCONFIG_HPP

#include "Location.hpp"
#include <string>
#include <vector>
#include <map>

class ServerConfig {
private:
    std::string _host;
    int _port;
    std::vector<std::string> _server_names;
    std::map<int, std::string> _error_pages;
    size_t _max_body_size;
    std::vector<Location> _locations;

public:
    ServerConfig();
    ~ServerConfig();
    
    // Getters
    const std::string& getHost() const;
    int getPort() const;
    const std::vector<std::string>& getServerNames() const;
    const std::map<int, std::string>& getErrorPages() const;
    size_t getMaxBodySize() const;
    const std::vector<Location>& getLocations() const;
    
    // Setters
    void setHost(const std::string& host);
    void setPort(int port);
    void setServerNames(const std::vector<std::string>& server_names);
    void setErrorPages(const std::map<int, std::string>& error_pages);
    void setMaxBodySize(size_t max_body_size);
    void setLocations(const std::vector<Location>& locations);
    void addServerName(const std::string& server_name);
    void addErrorPage(int code, const std::string& page);
    void addLocation(const Location& location);
    
    void print() const;
};

#endif