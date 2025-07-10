#include "ConnectionHandler.hpp"
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <errno.h>
#include <cstring>
#include <ctime>
#include <map>

/*
 * Default constructor for ConnectionHandler
 * Initializes the connection handler with empty client map
 */
ConnectionHandler::ConnectionHandler() : _server_configs(NULL) {}

/*
 * Destructor for ConnectionHandler
 * Closes all client connections and cleans up resources
 */
ConnectionHandler::~ConnectionHandler() {
    closeAllClients();
}

/*
 * Sets the server configurations for location matching
 */
void ConnectionHandler::setServerConfigs(const std::vector<ServerConfig>& configs) {
    _server_configs = &configs;
}

/*
 * Closes all client connections and clears the client map
 * Used for graceful shutdown
 */
void ConnectionHandler::closeAllClients() {
    
    for (std::map<int, ClientData>::iterator it = _clients.begin(); 
         it != _clients.end(); ++it) {
        _socket_manager.closeSocket(it->first);
    }
    _clients.clear();
}

/*
 * Checks for clients with empty request timeouts
 * Called periodically to handle clients that connect but don't send data
 * Returns list of clients that need POLLOUT events
 */
std::vector<int> ConnectionHandler::checkEmptyRequestTimeouts() {
    time_t current_time = time(NULL);
    std::vector<int> clients_needing_pollout;
    
    for (std::map<int, ClientData>::iterator it = _clients.begin(); 
         it != _clients.end(); ++it) {
        int client_sock = it->first;
        ClientData& client = it->second;
        
        // Check if client has been connected without sending data for too long
        if (client.getReadBuffer().empty() && client.getWriteBuffer().empty()) {
            time_t connection_time = client.getConnectionTime();
            time_t elapsed = current_time - connection_time;
            
            
            // 1 second timeout for empty requests (quick response)
            if (elapsed >= 1) {
                std::cout << "Empty request timeout from client " << client_sock << std::endl;
                
                HttpResponse response = HttpResponse::createBadRequestResponse();
                client.setWriteBuffer(response.toString());
                client.setBytesSent(0);
                
                // This client now needs POLLOUT events to send the response
                clients_needing_pollout.push_back(client_sock);
            }
        }
    }
    
    return clients_needing_pollout;
}

/*
 * Accepts a new client connection on a listening socket
 * Sets the client socket to non-blocking mode and creates ClientData
 * Returns the client socket file descriptor, or -1 on error
 */
int ConnectionHandler::acceptNewConnection(int listen_sock) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    int client_sock = accept(listen_sock, (struct sockaddr*)&client_addr, &client_len);
    if (client_sock < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            std::cerr << "Error accepting connection: " << strerror(errno) << std::endl;
        }
        return -1;
    }
    
    int flags = fcntl(client_sock, F_GETFL, 0);
    if (flags < 0 || fcntl(client_sock, F_SETFL, flags | O_NONBLOCK) < 0) {
        std::cerr << "Error setting client socket non-blocking: " << strerror(errno) << std::endl;
        _socket_manager.closeSocket(client_sock);
        return -1;
    }
    
    _clients[client_sock] = ClientData();
    
    std::string client_ip = SocketManager::ipToString(client_addr);
    std::cout << "New connection from " << client_ip << ":" << ntohs(client_addr.sin_port) 
              << " (fd: " << client_sock << ")" << std::endl;
    
    return client_sock;
}

/*
 * Processes incoming data from a client
 * Appends data to the client's read buffer and parses HTTP request
 * Creates appropriate HTTP response based on request
 */
void ConnectionHandler::processClientData(int client_sock, const char* buffer, ssize_t bytes_read) {
    _clients[client_sock].appendToReadBuffer(buffer, bytes_read);
    
    std::cout << "Received " << bytes_read << " bytes from client " << client_sock << std::endl;
    
    // Try to parse HTTP request from accumulated buffer
    HttpRequest request;
    std::string accumulated_data = _clients[client_sock].getReadBuffer();
    
    // Special handling for empty request (when client sends nothing and closes connection)
    if (bytes_read == 0 && accumulated_data.empty()) {
        std::cout << "Empty request from client " << client_sock << std::endl;
        HttpResponse response = HttpResponse::createBadRequestResponse();
        _clients[client_sock].setWriteBuffer(response.toString());
        _clients[client_sock].setBytesSent(0);
        _clients[client_sock].clearReadBuffer();
        return;
    }
    
    if (request.parse(accumulated_data)) {
        if (request.isValid()) {
            std::cout << "Valid HTTP request: " << request.getMethod() 
                      << " " << request.getUri() << " " << request.getVersion() << std::endl;
            
            // Process the HTTP request and generate response
            HttpResponse response = processHttpRequest(request);
            std::string response_str = response.toString();
            
            _clients[client_sock].setWriteBuffer(response_str);
            _clients[client_sock].setBytesSent(0);
            
            // Clear read buffer after successful parsing
            _clients[client_sock].clearReadBuffer();
        } else {
            std::cout << "Invalid HTTP request from client " << client_sock << std::endl;
            HttpResponse response;
            
            // Use specific error code from request parsing
            int error_code = request.getErrorCode();
            if (error_code == 411) {
                response = HttpResponse::createLengthRequiredResponse();
            } else {
                response = HttpResponse::createBadRequestResponse();
            }
            
            _clients[client_sock].setWriteBuffer(response.toString());
            _clients[client_sock].setBytesSent(0);
            _clients[client_sock].clearReadBuffer();
        }
    } else {
        // Check if this looks like a malformed request rather than incomplete
        // A complete request has header termination (\r\n\r\n or \n\n)
        bool has_header_termination = (accumulated_data.find("\r\n\r\n") != std::string::npos ||
                                      accumulated_data.find("\n\n") != std::string::npos);
        
        // Check for empty request or malformed request
        bool is_empty_request = (accumulated_data.empty());
        bool is_malformed = (accumulated_data.size() > 0 && accumulated_data.find(' ') == std::string::npos);
        
        if (has_header_termination || is_empty_request || is_malformed) {
            // This appears to be a malformed or empty request, not incomplete
            std::cout << "Malformed or empty HTTP request from client " << client_sock << std::endl;
            HttpResponse response = HttpResponse::createBadRequestResponse();
            _clients[client_sock].setWriteBuffer(response.toString());
            _clients[client_sock].setBytesSent(0);
            _clients[client_sock].clearReadBuffer();
        } else {
            // Request not complete yet, wait for more data
            std::cout << "Incomplete HTTP request, waiting for more data..." << std::endl;
        }
    }
}

/*
 * Processes an HTTP request and generates an appropriate response
 * Handles different HTTP methods and creates proper responses
 * Returns HttpResponse object with proper status codes and headers
 */
HttpResponse ConnectionHandler::processHttpRequest(const HttpRequest& request) {
    std::string method = request.getMethod();
    std::string uri = request.getUri();
    
    // Sanitize path to prevent directory traversal attacks
    std::string sanitized_uri = sanitizePath(uri);
    if (sanitized_uri.empty()) {
        return HttpResponse::createNotFoundResponse();  // Dangerous path detected - return 404 for security
    }
    
    // Find matching location
    const Location* location = findMatchingLocation(sanitized_uri);
    if (!location) {
        return HttpResponse::createNotFoundResponse();
    }
    
    // Check if method is allowed for this location
    const std::vector<std::string>& allowed_methods = location->getMethods();
    bool method_allowed = false;
    for (size_t i = 0; i < allowed_methods.size(); ++i) {
        if (allowed_methods[i] == method) {
            method_allowed = true;
            break;
        }
    }
    
    if (!method_allowed) {
        return HttpResponse::createMethodNotAllowedResponse();
    }
    
    // Handle GET requests with proper file serving logic
    if (method == "GET") {
        if (sanitized_uri == "/") {
            // Return a welcome page for root
            std::string body = "<html><body><h1>Welcome to WebServ</h1><p>HTTP/1.1 Server</p></body></html>";
            return HttpResponse::createOkResponse(body, "text/html");
        } else {
            // For other paths, determine MIME type and return appropriate response
            std::string detected_mime = getMimeType(sanitized_uri);
            std::string body = "<html><body><h1>Path Found: " + sanitized_uri + "</h1>";
            body += "<p>Location: " + location->getPath() + "</p>";
            body += "<p>Root: " + location->getRoot() + "</p>";
            body += "<p>Detected MIME Type: " + detected_mime + "</p>";
            body += "</body></html>";
            // Return HTML content with correct content-type
            return HttpResponse::createOkResponse(body, "text/html");
        }
    } else if (method == "POST") {
        std::string body = "POST request received\nURI: " + sanitized_uri + "\nBody: " + request.getBody();
        return HttpResponse::createOkResponse(body, "text/plain");
    } else if (method == "DELETE") {
        std::string body = "DELETE request received\nURI: " + sanitized_uri;
        return HttpResponse::createOkResponse(body, "text/plain");
    }
    
    return HttpResponse::createServerErrorResponse();
}

/*
 * Handles reading data from a client socket
 * Receives data, processes it, or handles connection errors/disconnects
 * Uses non-blocking I/O and handles EAGAIN/EWOULDBLOCK appropriately
 */
void ConnectionHandler::handleClientRead(int client_sock) {
    char buffer[4096];
    ssize_t bytes_read = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
    
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        processClientData(client_sock, buffer, bytes_read);
    } else if (bytes_read == 0) {
        // Client closed connection - check if we have any data to process
        std::string accumulated_data = _clients[client_sock].getReadBuffer();
        if (accumulated_data.empty()) {
            // Empty request - process as empty request
            processClientData(client_sock, "", 0);
            return; // Don't remove client yet, let it send the response
        }
        std::cout << "Client " << client_sock << " disconnected" << std::endl;
        removeClient(client_sock);
    } else {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            std::cerr << "Error reading from client " << client_sock 
                      << ": " << strerror(errno) << std::endl;
            removeClient(client_sock);
        }
    }
}

/*
 * Handles writing data to a client socket
 * Sends pending response data and tracks bytes sent
 * Removes client when response is fully sent or on error
 */
void ConnectionHandler::handleClientWrite(int client_sock) {
    ClientData& client = _clients[client_sock];
    
    if (client.getBytesSent() >= client.getWriteBuffer().size()) {
        return;
    }
    
    ssize_t bytes_sent = send(client_sock, 
                             client.getWriteBuffer().c_str() + client.getBytesSent(),
                             client.getWriteBuffer().size() - client.getBytesSent(), 0);
    
    if (bytes_sent > 0) {
        client.setBytesSent(client.getBytesSent() + bytes_sent);
        std::cout << "Sent " << bytes_sent << " bytes to client " << client_sock << std::endl;
        
        if (client.getBytesSent() >= client.getWriteBuffer().size()) {
            std::cout << "Finished sending response to client " << client_sock << std::endl;
            removeClient(client_sock);
        }
    } else if (bytes_sent == 0) {
        std::cout << "Client " << client_sock << " closed connection during write" << std::endl;
        removeClient(client_sock);
    } else {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            std::cerr << "Error writing to client " << client_sock 
                      << ": " << strerror(errno) << std::endl;
            removeClient(client_sock);
        }
    }
}

/*
 * Removes a client from the connection handler
 * Closes the socket and erases client data from the map
 */
void ConnectionHandler::removeClient(int client_sock) {
    _clients.erase(client_sock);
    _socket_manager.closeSocket(client_sock);
    std::cout << "Removed client " << client_sock << std::endl;
}

/*
 * Checks if a client socket exists in the handler
 * Returns true if client is managed by this handler
 */
bool ConnectionHandler::hasClient(int client_sock) const {
    return _clients.find(client_sock) != _clients.end();
}

/*
 * Returns a reference to the ClientData for the given socket
 * Non-const version for modifying client data
 */
ClientData& ConnectionHandler::getClient(int client_sock) {
    return _clients[client_sock];
}

/*
 * Returns a const reference to the ClientData for the given socket
 * Const version for read-only access to client data
 */
const ClientData& ConnectionHandler::getClient(int client_sock) const {
    std::map<int, ClientData>::const_iterator it = _clients.find(client_sock);
    return it->second;
}

/*
 * Finds the best matching location for the given URI
 * Returns pointer to matching Location or NULL if no match found
 */
const Location* ConnectionHandler::findMatchingLocation(const std::string& uri) const {
    if (!_server_configs || _server_configs->empty()) {
        return NULL;
    }
    
    // For now, use the first server config (can be enhanced for virtual hosts)
    const ServerConfig& config = (*_server_configs)[0];
    const std::vector<Location>& locations = config.getLocations();
    
    const Location* best_match = NULL;
    size_t best_match_length = 0;
    
    // Find the longest matching location path
    for (size_t i = 0; i < locations.size(); ++i) {
        const Location& location = locations[i];
        const std::string& location_path = location.getPath();
        
        // Check if URI matches location path
        if (location_path == "/") {
            // Root location only matches exactly "/"
            if (uri == "/") {
                if (location_path.length() > best_match_length) {
                    best_match = &location;
                    best_match_length = location_path.length();
                }
            }
        } else if (uri.find(location_path) == 0) {
            // Non-root locations: exact match or followed by '/'
            if (uri.length() == location_path.length() || 
                (uri.length() > location_path.length() && uri[location_path.length()] == '/')) {
                
                // Keep the longest match (most specific)
                if (location_path.length() > best_match_length) {
                    best_match = &location;
                    best_match_length = location_path.length();
                }
            }
        }
    }
    
    return best_match;
}

/*
 * Sanitizes and validates path to prevent directory traversal attacks
 * Returns the sanitized path or empty string if path is dangerous
 */
std::string ConnectionHandler::sanitizePath(const std::string& path) const {
    if (path.empty()) {
        return "";
    }
    
    // Check for obvious path traversal attempts
    if (path.find("../") != std::string::npos || path.find("..\\") != std::string::npos) {
        return "";  // Reject paths with parent directory references
    }
    
    // Check for other dangerous patterns
    if (path.find("//") != std::string::npos || path.find("\\") != std::string::npos) {
        return "";  // Reject paths with double slashes or backslashes
    }
    
    // Path must start with /
    if (path[0] != '/') {
        return "";
    }
    
    // Additional security: reject paths with null bytes or control characters
    for (size_t i = 0; i < path.length(); ++i) {
        char c = path[i];
        if (c < 32 && c != '\t') {  // Allow tab but reject other control chars
            return "";
        }
    }
    
    return path;  // Path is safe
}

/*
 * Determines MIME type based on file extension
 * Returns the appropriate MIME type string
 */
std::string ConnectionHandler::getMimeType(const std::string& path) const {
    // Find the last dot to get file extension
    size_t dot_pos = path.find_last_of('.');
    if (dot_pos == std::string::npos) {
        return "text/plain";  // Default for files without extension
    }
    
    std::string extension = path.substr(dot_pos);
    
    // Convert to lowercase for comparison
    for (size_t i = 0; i < extension.length(); ++i) {
        if (extension[i] >= 'A' && extension[i] <= 'Z') {
            extension[i] = extension[i] + 32;  // Convert to lowercase
        }
    }
    
    // Basic MIME type mapping
    if (extension == ".html" || extension == ".htm") {
        return "text/html";
    } else if (extension == ".css") {
        return "text/css";
    } else if (extension == ".js") {
        return "application/javascript";
    } else if (extension == ".png") {
        return "image/png";
    } else if (extension == ".jpg" || extension == ".jpeg") {
        return "image/jpeg";
    } else if (extension == ".gif") {
        return "image/gif";
    } else if (extension == ".txt") {
        return "text/plain";
    } else if (extension == ".json") {
        return "application/json";
    } else if (extension == ".xml") {
        return "application/xml";
    } else {
        return "application/octet-stream";  // Default for unknown types
    }
}