#include "ServerConfig.hpp"
#include <iostream>

ServerConfig::ServerConfig() : _host("127.0.0.1"), _port(80), _max_body_size(1024 * 1024) {}

ServerConfig::~ServerConfig() {}

// Getters
const std::string& ServerConfig::getHost() const {
    return _host;
}

int ServerConfig::getPort() const {
    return _port;
}

const std::vector<std::string>& ServerConfig::getServerNames() const {
    return _server_names;
}

const std::map<int, std::string>& ServerConfig::getErrorPages() const {
    return _error_pages;
}

size_t ServerConfig::getMaxBodySize() const {
    return _max_body_size;
}

const std::vector<Location>& ServerConfig::getLocations() const {
    return _locations;
}

// Setters
void ServerConfig::setHost(const std::string& host) {
    _host = host;
}

void ServerConfig::setPort(int port) {
    _port = port;
}

void ServerConfig::setServerNames(const std::vector<std::string>& server_names) {
    _server_names = server_names;
}

void ServerConfig::setErrorPages(const std::map<int, std::string>& error_pages) {
    _error_pages = error_pages;
}

void ServerConfig::setMaxBodySize(size_t max_body_size) {
    _max_body_size = max_body_size;
}

void ServerConfig::setLocations(const std::vector<Location>& locations) {
    _locations = locations;
}

void ServerConfig::addServerName(const std::string& server_name) {
    _server_names.push_back(server_name);
}

void ServerConfig::addErrorPage(int code, const std::string& page) {
    _error_pages[code] = page;
}

void ServerConfig::addLocation(const Location& location) {
    _locations.push_back(location);
}

void ServerConfig::print() const {
    std::cout << "Server Configuration:" << std::endl;
    std::cout << "  Host: " << _host << std::endl;
    std::cout << "  Port: " << _port << std::endl;
    std::cout << "  Server names: ";
    for (size_t i = 0; i < _server_names.size(); ++i) {
        std::cout << _server_names[i];
        if (i < _server_names.size() - 1) std::cout << " ";
    }
    std::cout << std::endl;
    std::cout << "  Max body size: " << _max_body_size << std::endl;
    std::cout << "  Error pages:" << std::endl;
    for (std::map<int, std::string>::const_iterator it = _error_pages.begin(); 
         it != _error_pages.end(); ++it) {
        std::cout << "    " << it->first << ": " << it->second << std::endl;
    }
    std::cout << "  Locations:" << std::endl;
    for (size_t i = 0; i < _locations.size(); ++i) {
        _locations[i].print();
    }
    std::cout << std::endl;
}