#include "SignalManager.hpp"
#include <iostream>
#include <cstdlib>

// Static member initialization
volatile sig_atomic_t SignalManager::_shutdown_requested = 0;

/*
 * Constructor
 */
SignalManager::SignalManager() {}

/*
 * Destructor - resets signal handlers to default
 */
SignalManager::~SignalManager() {
    resetSignals();
}

/*
 * Static signal callback function
 * Only sets the flag - async-signal-safe
 */
void SignalManager::signalCallback(int signum) {
    (void)signum; // Suppress unused parameter warning
    _shutdown_requested = 1;
}

/*
 * Setup signal handlers
 */
void SignalManager::setupSignals() {
    if (signal(SIGINT, signalCallback) == SIG_ERR) {
        std::cerr << "Error setting up SIGINT handler" << std::endl;
        exit(1);
    }
    
    if (signal(SIGTERM, signalCallback) == SIG_ERR) {
        std::cerr << "Error setting up SIGTERM handler" << std::endl;
        exit(1);
    }
    
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        std::cerr << "Error ignoring SIGPIPE" << std::endl;
        exit(1);
    }
}

/*
 * Reset signal handlers to default
 */
void SignalManager::resetSignals() {
    signal(SIGINT, SIG_DFL);
    signal(SIGTERM, SIG_DFL);
    signal(SIGPIPE, SIG_DFL);
}

/*
 * Check if shutdown was requested
 */
bool SignalManager::isShutdownRequested() const {
    return _shutdown_requested != 0;
}