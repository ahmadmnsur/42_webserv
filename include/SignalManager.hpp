#ifndef SIGNALMANAGER_HPP
#define SIGNALMANAGER_HPP

#include <signal.h>

class SignalManager {
private:
    static volatile sig_atomic_t _shutdown_requested;
    static void signalCallback(int signum);

public:
    SignalManager();
    ~SignalManager();
    
    void setupSignals();
    bool isShutdownRequested() const;
    void resetSignals(); // Reset to default handlers
};

#endif