#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include <string>
#include <map>
#include <sstream>
#include <vector>

class HttpResponse {
private:
    int _status_code;
    std::string _status_message;
    std::string _version;
    std::map<std::string, std::string> _headers;
    std::string _body;
    bool _is_head_response;
    
    std::string getStatusMessage(int status_code) const;

public:
    HttpResponse();
    ~HttpResponse();
    
    void setStatusCode(int status_code);
    void setVersion(const std::string& version);
    void setHeader(const std::string& name, const std::string& value);
    void setBody(const std::string& body);
    void setContentType(const std::string& content_type);
    void setContentLength(size_t length);
    void setConnection(bool keep_alive);
    
    // Getters
    int getStatusCode() const;
    const std::string& getVersion() const;
    const std::string& getBody() const;
    std::string getHeader(const std::string& name) const;
    
    // Generate response
    std::string toString() const;
    
    // Static factory methods for common responses
    static HttpResponse createOkResponse(const std::string& body, const std::string& content_type = "text/plain");
    static HttpResponse createHeadResponse(const std::string& content_type = "text/plain", size_t content_length = 0);
    static HttpResponse createNotFoundResponse();
    static HttpResponse createForbiddenResponse();
    static HttpResponse createServerErrorResponse();
    static HttpResponse createBadRequestResponse();
    static HttpResponse createMethodNotAllowedResponse();
    static HttpResponse createMethodNotAllowedResponse(const std::vector<std::string>& allowed_methods);
    static HttpResponse createLengthRequiredResponse();
    static HttpResponse createRequestTimeoutResponse();
    static HttpResponse createRequestEntityTooLargeResponse();
    
    void clear();
};

#endif