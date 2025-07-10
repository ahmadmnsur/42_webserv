#include "HttpResponse.hpp"
#include <sstream>

HttpResponse::HttpResponse() : _status_code(200), _version("HTTP/1.1") {
    _status_message = getStatusMessage(_status_code);
}

HttpResponse::~HttpResponse() {}

std::string HttpResponse::getStatusMessage(int status_code) const {
    switch (status_code) {
        case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 408: return "Request Timeout";
        case 411: return "Length Required";
        case 413: return "Payload Too Large";
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 502: return "Bad Gateway";
        case 503: return "Service Unavailable";
        default: return "Unknown";
    }
}

void HttpResponse::setStatusCode(int status_code) {
    _status_code = status_code;
    _status_message = getStatusMessage(status_code);
}

void HttpResponse::setVersion(const std::string& version) {
    _version = version;
}

void HttpResponse::setHeader(const std::string& name, const std::string& value) {
    _headers[name] = value;
}

void HttpResponse::setBody(const std::string& body) {
    _body = body;
    setContentLength(body.size());
}

void HttpResponse::setContentType(const std::string& content_type) {
    setHeader("Content-Type", content_type);
}

void HttpResponse::setContentLength(size_t length) {
    std::ostringstream oss;
    oss << length;
    setHeader("Content-Length", oss.str());
}

void HttpResponse::setConnection(bool keep_alive) {
    if (keep_alive) {
        setHeader("Connection", "keep-alive");
    } else {
        setHeader("Connection", "close");
    }
}

int HttpResponse::getStatusCode() const {
    return _status_code;
}

const std::string& HttpResponse::getVersion() const {
    return _version;
}

const std::string& HttpResponse::getBody() const {
    return _body;
}

std::string HttpResponse::getHeader(const std::string& name) const {
    std::map<std::string, std::string>::const_iterator it = _headers.find(name);
    if (it != _headers.end()) {
        return it->second;
    }
    return "";
}

std::string HttpResponse::toString() const {
    std::ostringstream response;
    
    // Status line
    response << _version << " " << _status_code << " " << _status_message << "\r\n";
    
    // Headers
    for (std::map<std::string, std::string>::const_iterator it = _headers.begin(); 
         it != _headers.end(); ++it) {
        response << it->first << ": " << it->second << "\r\n";
    }
    
    // Empty line to separate headers from body
    response << "\r\n";
    
    // Body
    response << _body;
    
    return response.str();
}

HttpResponse HttpResponse::createOkResponse(const std::string& body, const std::string& content_type) {
    HttpResponse response;
    response.setStatusCode(200);
    response.setContentType(content_type);
    response.setBody(body);
    response.setConnection(false);
    return response;
}

HttpResponse HttpResponse::createNotFoundResponse() {
    HttpResponse response;
    response.setStatusCode(404);
    response.setContentType("text/html");
    response.setBody("<html><body><h1>404 Not Found</h1><p>The requested resource was not found.</p></body></html>");
    response.setConnection(false);
    return response;
}

HttpResponse HttpResponse::createServerErrorResponse() {
    HttpResponse response;
    response.setStatusCode(500);
    response.setContentType("text/html");
    response.setBody("<html><body><h1>500 Internal Server Error</h1><p>The server encountered an internal error.</p></body></html>");
    response.setConnection(false);
    return response;
}

HttpResponse HttpResponse::createBadRequestResponse() {
    HttpResponse response;
    response.setStatusCode(400);
    response.setContentType("text/html");
    response.setBody("<html><body><h1>400 Bad Request</h1><p>The request was malformed.</p></body></html>");
    response.setConnection(false);
    return response;
}

HttpResponse HttpResponse::createMethodNotAllowedResponse() {
    HttpResponse response;
    response.setStatusCode(405);
    response.setContentType("text/html");
    response.setBody("<html><body><h1>405 Method Not Allowed</h1><p>The requested method is not allowed.</p></body></html>");
    response.setConnection(false);
    return response;
}

HttpResponse HttpResponse::createLengthRequiredResponse() {
    HttpResponse response;
    response.setStatusCode(411);
    response.setContentType("text/html");
    response.setBody("<html><body><h1>411 Length Required</h1><p>Content-Length header is required for this request.</p></body></html>");
    response.setConnection(false);
    return response;
}

HttpResponse HttpResponse::createRequestTimeoutResponse() {
    HttpResponse response;
    response.setStatusCode(408);
    response.setContentType("text/html");
    response.setBody("<html><body><h1>408 Request Timeout</h1><p>The request timed out.</p></body></html>");
    response.setConnection(false);
    return response;
}

void HttpResponse::clear() {
    _status_code = 200;
    _status_message = "OK";
    _version = "HTTP/1.1";
    _headers.clear();
    _body.clear();
}