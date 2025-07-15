#include "ConfigParser.hpp"
#include "WebServer.hpp"
#include "SignalManager.hpp"
#include <iostream>

/*
 * Main entry point for the webserv HTTP server program
 * Handles command line arguments, configuration parsing, and server startup
 */
int main(int argc, char* argv[]) {
    // Create signal manager on the stack (no memory leaks!)
    SignalManager signalManager;
    if (!signalManager.setupSignals()) {
        std::cerr << "Failed to setup signal handlers" << std::endl;
        return 1;
    }
    
    // Set default configuration file name
    std::string config_file = "webserv.conf";
    
    // Use command line argument if provided
    if (argc > 1) {
        config_file = argv[1];
    }
    
    // Display which configuration file is being used
    std::cout << "Using config file: " << config_file << std::endl;
    
    // Parse the configuration file
    ConfigParser parser;
    std::vector<ServerConfig> configs = parser.parse(config_file);
    
    // Check if there were any parsing errors
    if (parser.hasErrors()) {
        std::cerr << "Configuration file has errors. Please fix them and try again." << std::endl;
        return 1;
    }
    
    // Ensure at least one server configuration was found
    if (configs.empty()) {
        std::cerr << "No valid server configurations found!" << std::endl;
        return 1;
    }
    
    // Display parsed configuration information for debugging
    std::cout << "Parsed " << configs.size() << " server configuration(s):" << std::endl;
    for (size_t i = 0; i < configs.size(); ++i) {
        configs[i].print();
    }
    
    // Create and run the web server with dependency injection
    try {
        WebServer server(configs, signalManager);
        if (!server.isValid()) {
            std::cerr << "Failed to create server - no valid listening sockets!" << std::endl;
            return 1;
        }
        
        // Run the server
        server.run();
		   
    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
        return 1;
    }
    
    // SignalManager destructor will automatically reset signal handlers
    return 0;
}