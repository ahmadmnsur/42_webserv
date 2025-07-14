#include "HttpResponse.hpp"
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <cctype>

HttpResponse::HttpResponse() : _status_code(200), _version("HTTP/1.1"), _is_head_response(false) {
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
    
    // Body (only for non-HEAD responses)
    if (!_is_head_response) {
        response << _body;
    }
    
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

HttpResponse HttpResponse::createHeadResponse(const std::string& content_type, size_t content_length) {
    (void)content_length;  // Silence unused parameter warning
    HttpResponse response;
    response.setStatusCode(200);
    response.setContentType(content_type);
    response.setContentLength(0);  // Set Content-Length to 0 since no body is sent
    response.setConnection(false);
    response._is_head_response = true;
    // No body for HEAD responses
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

HttpResponse HttpResponse::createForbiddenResponse() {
    HttpResponse response;
    response.setStatusCode(403);
    response.setContentType("text/html");
    response.setBody("<html><body><h1>403 Forbidden</h1><p>Access to this resource is forbidden.</p></body></html>");
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
    response.setHeader("Allow", "GET");
    response.setBody("<html><body><h1>405 Method Not Allowed</h1><p>The requested method is not allowed.</p></body></html>");
    response.setConnection(false);
    return response;
}

HttpResponse HttpResponse::createMethodNotAllowedResponse(const std::vector<std::string>& allowed_methods) {
    HttpResponse response;
    response.setStatusCode(405);
    response.setContentType("text/html");
    
    // Build Allow header from allowed methods, automatically include HEAD if GET is allowed
    std::string allow_header;
    for (size_t i = 0; i < allowed_methods.size(); ++i) {
        if (i > 0) allow_header += ", ";
        allow_header += allowed_methods[i];
    }
    
    // Add HEAD if GET is allowed but HEAD is not explicitly listed
    bool has_get = false, has_head = false;
    for (size_t i = 0; i < allowed_methods.size(); ++i) {
        if (allowed_methods[i] == "GET") has_get = true;
        if (allowed_methods[i] == "HEAD") has_head = true;
    }
    if (has_get && !has_head) {
        if (!allow_header.empty()) allow_header += ", ";
        allow_header += "HEAD";
    }
    
    response.setHeader("Allow", allow_header);
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

HttpResponse HttpResponse::createRequestEntityTooLargeResponse() {
    HttpResponse response;
    response.setStatusCode(413);
    response.setContentType("text/html");
    response.setBody("<html><body><h1>413 Payload Too Large</h1><p>The request payload is too large.</p></body></html>");
    response.setConnection(false);
    return response;
}

HttpResponse HttpResponse::createRedirectResponse(const std::string& redirect_info) {
    HttpResponse response;
    
    // Parse redirect info: either "301 URL" or just "URL" (defaults to 301)
    int status_code = 301;  // Default redirect status
    std::string url = redirect_info;
    
    // Check if the redirect_info starts with a status code
    size_t space_pos = redirect_info.find(' ');
    if (space_pos != std::string::npos) {
        std::string status_str = redirect_info.substr(0, space_pos);
        // Check if it's a valid 3xx status code
        if (status_str.length() == 3 && status_str[0] == '3' && 
            std::isdigit(status_str[1]) && std::isdigit(status_str[2])) {
            status_code = std::atoi(status_str.c_str());
            url = redirect_info.substr(space_pos + 1);
        }
    }
    
    response.setStatusCode(status_code);
    response.setHeader("Location", url);
    response.setContentType("text/html");
    response.setBody("<html><body><h1>" + response.getStatusMessage(status_code) + "</h1><p>The document has moved <a href=\"" + url + "\">here</a>.</p></body></html>");
    response.setConnection(false);
    return response;
}

void HttpResponse::clear() {
    _status_code = 200;
    _status_message = "OK";
    _version = "HTTP/1.1";
    _headers.clear();
    _body.clear();
    _is_head_response = false;
}