#include "ClientData.hpp"

ClientData::ClientData() : _bytes_sent(0) {}

ClientData::~ClientData() {}

// Getters
const std::string& ClientData::getReadBuffer() const {
    return _read_buffer;
}

const std::string& ClientData::getWriteBuffer() const {
    return _write_buffer;
}

size_t ClientData::getBytesSent() const {
    return _bytes_sent;
}

// Setters
void ClientData::setReadBuffer(const std::string& buffer) {
    _read_buffer = buffer;
}

void ClientData::setWriteBuffer(const std::string& buffer) {
    _write_buffer = buffer;
}

void ClientData::setBytesSent(size_t bytes_sent) {
    _bytes_sent = bytes_sent;
}

// Buffer operations
void ClientData::appendToReadBuffer(const std::string& data) {
    _read_buffer += data;
}

void ClientData::appendToReadBuffer(const char* data, size_t size) {
    _read_buffer.append(data, size);
}

void ClientData::clearReadBuffer() {
    _read_buffer.clear();
}

void ClientData::clearWriteBuffer() {
    _write_buffer.clear();
}