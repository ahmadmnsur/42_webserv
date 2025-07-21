#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include <string>
#include <map>
#include <vector>

class HttpRequest {
private:
    std::string _method;
    std::string _uri;
    std::string _version;
    std::map<std::string, std::string> _headers;
    std::string _body;
    bool _is_complete;
    bool _is_valid;
    int _error_code; // 0 = no error, 400 = bad request, 405 = method not allowed, 411 = length required
    size_t _bytes_consumed; // Track how many bytes were consumed during parsing
    
    std::string toLowerCase(const std::string& str) const;
    std::string trim(const std::string& str) const;
    bool parseRequestLine(const std::string& line);
    bool parseHeader(const std::string& line);
    bool isValidMethod(const std::string& method) const;
    bool isValidVersion(const std::string& version) const;
    bool validatePostRequest() const;

public:
    HttpRequest();
    ~HttpRequest();
    
    bool parse(const std::string& raw_request);
    void clear();
    
    // Getters
    const std::string& getMethod() const;
    const std::string& getUri() const;
    const std::string& getVersion() const;
    const std::map<std::string, std::string>& getHeaders() const;
    const std::string& getBody() const;
    bool isComplete() const;
    bool isValid() const;
    
    // Header utilities
    std::string getHeader(const std::string& name) const;
    bool hasHeader(const std::string& name) const;
    size_t getContentLength() const;
    bool isKeepAlive() const;
    int getErrorCode() const;
    size_t getBytesConsumed() const;
};

#endif