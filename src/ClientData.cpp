#include "ClientData.hpp"
#include <ctime>

/*
 * Default constructor for ClientData
 * Initializes with empty buffers and zero bytes sent
 */
ClientData::ClientData() : _bytes_sent(0), _connection_time(time(NULL)), _keep_alive(false) {}

/*
 * Destructor for ClientData
 * Cleans up any allocated resources
 */
ClientData::~ClientData() {}

// Getters
/*
 * Returns the read buffer containing received data from the client
 * Used for accessing incoming HTTP request data
 */
const std::string& ClientData::getReadBuffer() const {
    return _read_buffer;
}

/*
 * Returns the write buffer containing data to be sent to the client
 * Used for accessing outgoing HTTP response data
 */
const std::string& ClientData::getWriteBuffer() const {
    return _write_buffer;
}

/*
 * Returns the number of bytes already sent to the client
 * Used for tracking partial write progress
 */
size_t ClientData::getBytesSent() const {
    return _bytes_sent;
}

/*
 * Returns the connection time when the client connected
 * Used for timeout handling
 */
time_t ClientData::getConnectionTime() const {
    return _connection_time;
}

// Setters
/*
 * Sets the read buffer with new data
 * Used for replacing the entire read buffer contents
 */
void ClientData::setReadBuffer(const std::string& buffer) {
    _read_buffer = buffer;
}

/*
 * Sets the write buffer with new data
 * Used for preparing HTTP response data for sending
 */
void ClientData::setWriteBuffer(const std::string& buffer) {
    _write_buffer = buffer;
}

/*
 * Sets the number of bytes sent to the client
 * Used for tracking write progress during partial sends
 */
void ClientData::setBytesSent(size_t bytes_sent) {
    _bytes_sent = bytes_sent;
}

// Buffer operations
/*
 * Appends string data to the read buffer
 * Used for accumulating incoming request data
 */
void ClientData::appendToReadBuffer(const std::string& data) {
    _read_buffer += data;
}

/*
 * Appends raw character data to the read buffer
 * Used for accumulating incoming request data from socket reads
 */
void ClientData::appendToReadBuffer(const char* data, size_t size) {
    _read_buffer.append(data, size);
}

/*
 * Clears the read buffer
 * Used for resetting the client's incoming data buffer
 */
void ClientData::clearReadBuffer() {
    _read_buffer.clear();
}

/*
 * Clears the write buffer
 * Used for resetting the client's outgoing data buffer
 */
void ClientData::clearWriteBuffer() {
    _write_buffer.clear();
}

/*
 * Returns whether this connection should be kept alive
 * Used for HTTP/1.1 keep-alive connection handling
 */
bool ClientData::isKeepAlive() const {
    return _keep_alive;
}

/*
 * Sets the keep-alive flag for this connection
 * Used to control whether connection should be closed after response
 */
void ClientData::setKeepAlive(bool keep_alive) {
    _keep_alive = keep_alive;
}