#include "ConfigParser.hpp"
#include "WebServer.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    std::string config_file = "webserv.conf";
    
    if (argc > 1) {
        config_file = argv[1];
    }
    
    std::cout << "Using config file: " << config_file << std::endl;
    
    ConfigParser parser;
    std::vector<ServerConfig> configs = parser.parse(config_file);
    
    if (parser.hasErrors()) {
        std::cerr << "Configuration file has errors. Please fix them and try again." << std::endl;
        return 1;
    }
    
    if (configs.empty()) {
        std::cerr << "No valid server configurations found!" << std::endl;
        return 1;
    }
    
    std::cout << "Parsed " << configs.size() << " server configuration(s):" << std::endl;
    for (size_t i = 0; i < configs.size(); ++i) {
        configs[i].print();
    }
    
    try {
        WebServer server(configs);
        if (!server.isValid()) {
            std::cerr << "Failed to create server - no valid listening sockets!" << std::endl;
            return 1;
        }
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}