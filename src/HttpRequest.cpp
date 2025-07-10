#include "HttpRequest.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>

HttpRequest::HttpRequest() : _is_complete(false), _is_valid(false) {}

HttpRequest::~HttpRequest() {}

std::string HttpRequest::toLowerCase(const std::string& str) const {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

std::string HttpRequest::trim(const std::string& str) const {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

bool HttpRequest::isValidMethod(const std::string& method) const {
    return (method == "GET" || method == "POST" || method == "DELETE" || 
            method == "PUT" || method == "HEAD" || method == "OPTIONS");
}

bool HttpRequest::isValidVersion(const std::string& version) const {
    return (version == "HTTP/1.0" || version == "HTTP/1.1");
}

bool HttpRequest::parseRequestLine(const std::string& line) {
    std::istringstream iss(line);
    std::string method, uri, version;
    
    if (!(iss >> method >> uri >> version)) {
        return false;
    }
    
    if (!isValidMethod(method)) {
        return false;
    }
    
    if (!isValidVersion(version)) {
        return false;
    }
    
    if (uri.empty() || uri[0] != '/') {
        return false;
    }
    
    _method = method;
    _uri = uri;
    _version = version;
    
    return true;
}

bool HttpRequest::parseHeader(const std::string& line) {
    size_t colon_pos = line.find(':');
    if (colon_pos == std::string::npos) {
        return false;
    }
    
    std::string name = trim(line.substr(0, colon_pos));
    std::string value = trim(line.substr(colon_pos + 1));
    
    if (name.empty()) {
        return false;
    }
    
    _headers[toLowerCase(name)] = value;
    return true;
}

bool HttpRequest::parse(const std::string& raw_request) {
    clear();
    
    if (raw_request.empty()) {
        return false;
    }
    
    std::istringstream stream(raw_request);
    std::string line;
    
    // Parse request line
    if (!std::getline(stream, line)) {
        return false;
    }
    
    // Remove carriage return if present
    if (!line.empty() && line[line.size() - 1] == '\r') {
        line.erase(line.size() - 1);
    }
    
    if (!parseRequestLine(line)) {
        return false;
    }
    
    // Parse headers
    while (std::getline(stream, line)) {
        if (!line.empty() && line[line.size() - 1] == '\r') {
            line.erase(line.size() - 1);
        }
        
        if (line.empty()) {
            // Empty line indicates end of headers
            break;
        }
        
        if (!parseHeader(line)) {
            return false;
        }
    }
    
    // Read body if present
    std::ostringstream body_stream;
    std::string body_line;
    while (std::getline(stream, body_line)) {
        body_stream << body_line << "\n";
    }
    
    _body = body_stream.str();
    if (!_body.empty() && _body[_body.size() - 1] == '\n') {
        _body.erase(_body.size() - 1);
    }
    
    // Check if request is complete
    size_t content_length = getContentLength();
    if (content_length > 0) {
        _is_complete = (_body.size() >= content_length);
    } else {
        _is_complete = true;
    }
    
    // HTTP/1.1 requires Host header (RFC 7230 section 5.4)
    if (_version == "HTTP/1.1" && !hasHeader("host")) {
        _is_valid = false;
        return false;
    }
    
    _is_valid = true;
    return true;
}

void HttpRequest::clear() {
    _method.clear();
    _uri.clear();
    _version.clear();
    _headers.clear();
    _body.clear();
    _is_complete = false;
    _is_valid = false;
}

const std::string& HttpRequest::getMethod() const {
    return _method;
}

const std::string& HttpRequest::getUri() const {
    return _uri;
}

const std::string& HttpRequest::getVersion() const {
    return _version;
}

const std::map<std::string, std::string>& HttpRequest::getHeaders() const {
    return _headers;
}

const std::string& HttpRequest::getBody() const {
    return _body;
}

bool HttpRequest::isComplete() const {
    return _is_complete;
}

bool HttpRequest::isValid() const {
    return _is_valid;
}

std::string HttpRequest::getHeader(const std::string& name) const {
    std::map<std::string, std::string>::const_iterator it = _headers.find(toLowerCase(name));
    if (it != _headers.end()) {
        return it->second;
    }
    return "";
}

bool HttpRequest::hasHeader(const std::string& name) const {
    return _headers.find(toLowerCase(name)) != _headers.end();
}

size_t HttpRequest::getContentLength() const {
    std::string length_str = getHeader("content-length");
    if (length_str.empty()) {
        return 0;
    }
    
    std::istringstream iss(length_str);
    size_t length;
    if (!(iss >> length)) {
        return 0;
    }
    
    return length;
}

bool HttpRequest::isKeepAlive() const {
    std::string connection = getHeader("connection");
    std::string connection_lower = toLowerCase(connection);
    
    if (_version == "HTTP/1.1") {
        return connection_lower != "close";
    } else {
        return connection_lower == "keep-alive";
    }
}