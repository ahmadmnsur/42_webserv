#include "ServerConfig.hpp"
#include <iostream>

/*
 * Default constructor for ServerConfig
 * Initializes with default values: localhost, port 80, 1MB max body size
 */
ServerConfig::ServerConfig() : _host("127.0.0.1"), _port(80), _max_body_size(1024 * 1024) {}

/*
 * Destructor for ServerConfig
 * Cleans up any allocated resources
 */
ServerConfig::~ServerConfig() {}

// Getters
/*
 * Returns the configured host/IP address
 * Used for binding the listening socket
 */
const std::string& ServerConfig::getHost() const {
    return _host;
}

/*
 * Returns the configured port number
 * Used for binding the listening socket
 */
int ServerConfig::getPort() const {
    return _port;
}

/*
 * Returns the list of server names for this virtual host
 * Used for HTTP Host header matching
 */
const std::vector<std::string>& ServerConfig::getServerNames() const {
    return _server_names;
}

/*
 * Returns the map of error codes to custom error pages
 * Used for serving custom error responses
 */
const std::map<int, std::string>& ServerConfig::getErrorPages() const {
    return _error_pages;
}

/*
 * Returns the maximum allowed request body size in bytes
 * Used for limiting client upload sizes
 */
size_t ServerConfig::getMaxBodySize() const {
    return _max_body_size;
}

/*
 * Returns the list of location blocks for this server
 * Used for route-specific configuration
 */
const std::vector<Location>& ServerConfig::getLocations() const {
    return _locations;
}

// Setters
/*
 * Sets the host/IP address for this server configuration
 * Called during configuration parsing
 */
void ServerConfig::setHost(const std::string& host) {
    _host = host;
}

/*
 * Sets the port number for this server configuration
 * Called during configuration parsing
 */
void ServerConfig::setPort(int port) {
    _port = port;
}

/*
 * Sets the list of server names for this virtual host
 * Called during configuration parsing
 */
void ServerConfig::setServerNames(const std::vector<std::string>& server_names) {
    _server_names = server_names;
}

/*
 * Sets the map of error codes to custom error pages
 * Called during configuration parsing
 */
void ServerConfig::setErrorPages(const std::map<int, std::string>& error_pages) {
    _error_pages = error_pages;
}

/*
 * Sets the maximum allowed request body size
 * Called during configuration parsing
 */
void ServerConfig::setMaxBodySize(size_t max_body_size) {
    _max_body_size = max_body_size;
}

/*
 * Sets the list of location blocks for this server
 * Called during configuration parsing
 */
void ServerConfig::setLocations(const std::vector<Location>& locations) {
    _locations = locations;
}

/*
 * Adds a server name to the list of virtual host names
 * Called when parsing multiple server_name directives
 */
void ServerConfig::addServerName(const std::string& server_name) {
    _server_names.push_back(server_name);
}

/*
 * Adds a custom error page for a specific HTTP status code
 * Called when parsing error_page directives
 */
void ServerConfig::addErrorPage(int code, const std::string& page) {
    _error_pages[code] = page;
}

/*
 * Adds a location block to this server configuration
 * Called when parsing location directives
 */
void ServerConfig::addLocation(const Location& location) {
    _locations.push_back(location);
}

/*
 * Prints the server configuration to stdout for debugging
 * Displays all configured values including locations and error pages
 */
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