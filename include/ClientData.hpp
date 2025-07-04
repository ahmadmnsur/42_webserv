#ifndef CLIENTDATA_HPP
#define CLIENTDATA_HPP

#include <string>

class ClientData {
private:
    std::string _read_buffer;
    std::string _write_buffer;
    size_t _bytes_sent;

public:
    ClientData();
    ~ClientData();
    
    // Getters
    const std::string& getReadBuffer() const;
    const std::string& getWriteBuffer() const;
    size_t getBytesSent() const;
    
    // Setters
    void setReadBuffer(const std::string& buffer);
    void setWriteBuffer(const std::string& buffer);
    void setBytesSent(size_t bytes_sent);
    
    // Buffer operations
    void appendToReadBuffer(const std::string& data);
    void appendToReadBuffer(const char* data, size_t size);
    void clearReadBuffer();
    void clearWriteBuffer();
};

#endif