#ifndef CLIENTDATA_HPP
#define CLIENTDATA_HPP

#include <string>
#include <ctime>

class ClientData {
private:
    std::string _read_buffer;
    std::string _write_buffer;
    size_t _bytes_sent;
    time_t _connection_time;
    time_t _last_activity_time;
    bool _keep_alive;

public:
    ClientData();
    ~ClientData();
    
    // Getters
    const std::string& getReadBuffer() const;
    const std::string& getWriteBuffer() const;
    size_t getBytesSent() const;
    time_t getConnectionTime() const;
    time_t getLastActivityTime() const;
    bool isKeepAlive() const;
    
    // Setters
    void setReadBuffer(const std::string& buffer);
    void setWriteBuffer(const std::string& buffer);
    void setBytesSent(size_t bytes_sent);
    void setKeepAlive(bool keep_alive);
    void resetConnectionTime();
    void updateLastActivity();
    
    // Buffer operations
    void appendToReadBuffer(const std::string& data);
    void appendToReadBuffer(const char* data, size_t size);
    void clearReadBuffer();
    void clearWriteBuffer();
};

#endif