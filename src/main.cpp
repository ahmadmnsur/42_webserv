#include "ConfigParser.hpp"
#include "WebServer.hpp"
#include <iostream>
#include <signal.h>
#include <cstdlib>

// Global pointer to the server instance for signal handler access
WebServer* g_server = NULL;
volatile sig_atomic_t g_shutdown_requested = 0;

/*
 * Signal handler for SIGINT (Ctrl+C) and SIGTERM
 * Sets the shutdown flag - only async-signal-safe operations allowed
 */
void signalHandler(int signum) {
    // Only async-signal-safe operations in signal handler
    (void)signum; // Suppress unused parameter warning
    g_shutdown_requested = 1;
    
    // Note: We cannot call server->shutdown() here as it's not async-signal-safe
    // The main loop will check the flag and handle shutdown
}

/*
 * Sets up signal handlers for graceful shutdown
 * Uses signal() function which is in the allowed list
 */
void setupSignalHandlers() {
    if (signal(SIGINT, signalHandler) == SIG_ERR) {
        std::cerr << "Error setting up SIGINT handler" << std::endl;
        exit(1);
    }
    
    if (signal(SIGTERM, signalHandler) == SIG_ERR) {
        std::cerr << "Error setting up SIGTERM handler" << std::endl;
        exit(1);
    }
    
    // Ignore SIGPIPE to handle broken connections gracefully
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        std::cerr << "Error ignoring SIGPIPE" << std::endl;
        exit(1);
    }
}

/*
 * Main entry point for the webserv HTTP server program
 * Handles command line arguments, configuration parsing, and server startup
 */
int main(int argc, char* argv[]) {
    // Set up signal handlers first
    setupSignalHandlers();
    
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
    
    // Create and run the web server
    try {
        WebServer server(configs);
        if (!server.isValid()) {
            std::cerr << "Failed to create server - no valid listening sockets!" << std::endl;
            return 1;
        }
        
        // Set global server pointer for signal handler reference
        g_server = &server;
        
        // Run the server (will check g_shutdown_requested flag)
        server.run();
        
        // Clear global pointer
        g_server = NULL;
        
        std::cout << "Server shutdown complete." << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
        g_server = NULL;
        return 1;
    }
    
    return 0;
}