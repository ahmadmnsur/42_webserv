#include "ConnectionHandler.hpp"
#include <iostream>
#include <sstream>
#include <fstream>
#include <unistd.h>
#include <errno.h>
#include <cstring>
#include <ctime>
#include <map>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/wait.h>
#include <cstdlib>
#include <cstdio>

extern char** environ;

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
        
        time_t connection_time = client.getConnectionTime();
        time_t elapsed = current_time - connection_time;
        
        // Check if client has been connected without sending data for too long
        if (client.getReadBuffer().empty() && client.getWriteBuffer().empty()) {
            // 1 second timeout for empty requests (quick response)
            if (elapsed >= 1) {
                std::cout << "Empty request timeout from client " << client_sock << std::endl;
                
                HttpResponse response = HttpResponse::createBadRequestResponse();
                client.setWriteBuffer(response.toString());
                client.setBytesSent(0);
                
                // This client now needs POLLOUT events to send the response
                clients_needing_pollout.push_back(client_sock);
            }
        } else if (!client.getReadBuffer().empty() && client.getWriteBuffer().empty()) {
            // Check for incomplete requests (have data but no response ready)
            // Try to parse the accumulated data to see if it's an incomplete request
            HttpRequest request;
            std::string accumulated_data = client.getReadBuffer();
            
            if (request.parse(accumulated_data)) {
                if (request.isValid() && !request.isComplete()) {
                    // This is a valid but incomplete request - check for timeout
                    std::cout << "Found incomplete request from client " << client_sock 
                              << " (elapsed: " << elapsed << "s)" << std::endl;
                    
                    // 3 second timeout for incomplete body
                    if (elapsed >= 3) {
                        std::cout << "Incomplete request timeout from client " << client_sock << std::endl;
                        
                        HttpResponse response = HttpResponse::createRequestTimeoutResponse();
                        client.setWriteBuffer(response.toString());
                        client.setBytesSent(0);
                        
                        // This client now needs POLLOUT events to send the response
                        clients_needing_pollout.push_back(client_sock);
                    }
                }
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
        // accept() failed, return without checking errno
        std::cerr << "Error accepting connection" << std::endl;
        return -1;
    }
    
    int flags = fcntl(client_sock, F_GETFL, 0);
    if (flags < 0 || fcntl(client_sock, F_SETFL,
         flags | O_NONBLOCK) < 0) {
        std::cerr << "Error setting client socket non-blocking" << std::endl;
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
            // Check Content-Length against max_body_size before reading complete body
            if (request.hasHeader("Content-Length") || request.hasHeader("content-length")) {
                size_t content_length = request.getContentLength();
                const ServerConfig* server_config = getCurrentServerConfig(client_sock);
                if (server_config && content_length > server_config->getMaxBodySize()) {
                    std::cout << "Request body too large: " << content_length 
                              << " > " << server_config->getMaxBodySize() << std::endl;
                    
                    // Return 413 immediately without reading the body
                    HttpResponse response = HttpResponse::createRequestEntityTooLargeResponse();
                    _clients[client_sock].setWriteBuffer(response.toString());
                    _clients[client_sock].setBytesSent(0);
                    _clients[client_sock].clearReadBuffer();
                    return;
                }
            }
            
            if (request.isComplete()) {
                std::cout << "Complete HTTP request: " << request.getMethod() 
                          << " " << request.getUri() << " " << request.getVersion() << std::endl;
                
                // Process the HTTP request and generate response
                HttpResponse response = processHttpRequest(request);
                
                // Handle keep-alive connections
                bool should_keep_alive = request.isKeepAlive();
                if (should_keep_alive) {
                    response.setConnection(true);  // Set keep-alive
                    _clients[client_sock].setKeepAlive(true);
                } else {
                    response.setConnection(false); // Set close
                    _clients[client_sock].setKeepAlive(false);
                }
                
                std::string response_str = response.toString();
                
                _clients[client_sock].setWriteBuffer(response_str);
                _clients[client_sock].setBytesSent(0);
                
                // Clear read buffer after successful parsing
                _clients[client_sock].clearReadBuffer();
            } else {
                // Valid request but incomplete (waiting for body)
                std::cout << "Valid but incomplete HTTP request, waiting for body..." << std::endl;
                // Check for immediate timeout for testing purposes
                time_t current_time = time(NULL);
                time_t connection_time = _clients[client_sock].getConnectionTime();
                if (current_time - connection_time >= 3) {
                    std::cout << "Incomplete request immediate timeout from client " << client_sock << std::endl;
                    HttpResponse response = HttpResponse::createRequestTimeoutResponse();
                    _clients[client_sock].setWriteBuffer(response.toString());
                    _clients[client_sock].setBytesSent(0);
                    _clients[client_sock].clearReadBuffer();
                }
                // Timeout handling is also done in checkEmptyRequestTimeouts()
            }
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
        // Only check for malformed if we have header termination - don't reject partial data
        bool is_malformed = false;
        if (has_header_termination) {
            // Check if the first line looks like a valid HTTP request
            size_t first_line_end = accumulated_data.find('\n');
            if (first_line_end != std::string::npos) {
                std::string first_line = accumulated_data.substr(0, first_line_end);
                // Remove \r if present
                if (!first_line.empty() && first_line[first_line.length() - 1] == '\r') {
                    first_line.erase(first_line.length() - 1);
                }
                // Check if first line has at least 2 spaces (METHOD URI HTTP/VERSION)
                size_t first_space = first_line.find(' ');
                size_t second_space = (first_space != std::string::npos) ? first_line.find(' ', first_space + 1) : std::string::npos;
                is_malformed = (first_space == std::string::npos || second_space == std::string::npos);
                

             // Also check for invalid methods if the structure is correct
             if (!is_malformed && first_space != std::string::npos) {
                 std::string method = first_line.substr(0, first_space);
                 // Check if method contains invalid characters or is not a recognized method
                 if (method.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZ") != std::string::npos ||
                     (method != "GET" && method != "POST" && method != "DELETE" && method != "HEAD" && 
                      method != "PUT" && method != "PATCH" && method != "OPTIONS" && method != "TRACE" && 
                      method != "CONNECT" && method != "PROPFIND")) {
                     is_malformed = true;
                 }
             }
            }
        }
        
        if (is_empty_request || is_malformed) {
            // This appears to be a malformed or empty request, not incomplete
            std::cout << "Malformed or empty HTTP request from client " << client_sock << std::endl;
            HttpResponse response = HttpResponse::createBadRequestResponse();
            _clients[client_sock].setWriteBuffer(response.toString());
            _clients[client_sock].setBytesSent(0);
            _clients[client_sock].clearReadBuffer();
        } else {
            // Request not complete yet, check for timeout on incomplete requests
            std::cout << "Incomplete HTTP request, waiting for more data..." << std::endl;
            
            // Timeout handling for incomplete requests is now done in checkEmptyRequestTimeouts()
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
        return HttpResponse::createBadRequestResponse();  // Malformed path - return 400 Bad Request
    }
    
    // Find matching location
    const Location* location = findMatchingLocation(sanitized_uri);
    if (!location) {
        return createErrorResponse(404);
    }
    
    // Check for redirect before processing methods
    if (!location->getRedirect().empty()) {
        return HttpResponse::createRedirectResponse(location->getRedirect());
    }
    
    // Body size validation is now handled earlier in processClientData
    
    // Check if method is allowed for this location
    const std::vector<std::string>& allowed_methods = location->getMethods();
    bool method_allowed = false;
    bool get_allowed = false;
    
    for (size_t i = 0; i < allowed_methods.size(); ++i) {
        if (allowed_methods[i] == method) {
            method_allowed = true;
            break;
        }
        if (allowed_methods[i] == "GET") {
            get_allowed = true;
        }
    }
    
    // Allow HEAD if GET is allowed
    if (!method_allowed && method == "HEAD" && get_allowed) {
        method_allowed = true;
    }
    
    if (!method_allowed) {
        return HttpResponse::createMethodNotAllowedResponse(allowed_methods);
    }
    
    // Handle GET and HEAD requests with proper file serving logic
    if (method == "GET" || method == "HEAD") {
            // Construct the full file path
            std::string file_path = location->getRoot();
            if (file_path.empty() || file_path[file_path.length() - 1] != '/') {
                file_path += "/";
            }
            
            // Nginx-style path construction: simply concatenate root + URI
            file_path += sanitized_uri;
            
            // Check if the path exists
            struct stat path_stat;
            if (stat(file_path.c_str(), &path_stat) == 0) {
                // Path exists, check if it's a directory
                if (S_ISDIR(path_stat.st_mode)) {
                    // It's a directory - check for index files or show directory listing
                    std::string index_file_path = file_path;
                    if (index_file_path[index_file_path.length() - 1] != '/') {
                        index_file_path += "/";
                    }
                    
                    // Check for index files
                    const std::vector<std::string>& index_files = location->getIndexFiles();
                    for (std::vector<std::string>::const_iterator it = index_files.begin(); 
                         it != index_files.end(); ++it) {
                        std::string index_path = index_file_path + *it;
                        if (access(index_path.c_str(), F_OK) == 0) {
                            // Index file found, serve it
                            std::string detected_mime = getMimeType(*it);
                            
                            // Read the actual file content
                            std::ifstream file(index_path.c_str(), std::ios::binary);
                            if (file.is_open()) {
                                std::string file_content((std::istreambuf_iterator<char>(file)),
                                                        std::istreambuf_iterator<char>());
                                file.close();
                                return HttpResponse::createOkResponse(file_content, detected_mime);
                            } else {
                                return HttpResponse::createForbiddenResponse();
                            }
                        }
                    }
                    
                    // No index file found - check if autoindex is enabled
                    if (location->getAutoindex()) {
                        // Generate directory listing
                        std::string body = "<html><head><title>Directory listing for " + sanitized_uri + "</title></head><body>";
                        body += "<h1>Directory listing for " + sanitized_uri + "</h1><hr>";
                        body += "<ul>";
                        
                        DIR* dir = opendir(file_path.c_str());
                        if (dir) {
                            struct dirent* entry;
                            while ((entry = readdir(dir)) != NULL) {
                                std::string name = entry->d_name;
                                if (name != "." && name != "..") {
                                    std::string href = sanitized_uri;
                                    if (href[href.length() - 1] != '/') href += "/";
                                    href += name;
                                    body += "<li><a href=\"" + href + "\">" + name + "</a></li>";
                                }
                            }
                            closedir(dir);
                        }
                        
                        body += "</ul><hr></body></html>";
                        return HttpResponse::createOkResponse(body, "text/html");
                    } else {
                        return createErrorResponse(403);
                    }
                } else {
                    // It's a regular file - check if it's a CGI script first
                    std::string file_extension;
                    size_t dot_pos = sanitized_uri.find_last_of('.');
                    if (dot_pos != std::string::npos) {
                        file_extension = sanitized_uri.substr(dot_pos);
                    }
                    
                    // Get CGI extensions from location
                    const std::map<std::string, std::string>& cgi_extensions = location->getCgiExtensions();
                    std::map<std::string, std::string>::const_iterator cgi_it = cgi_extensions.find(file_extension);
                    
                    if (cgi_it != cgi_extensions.end()) {
                        // This is a CGI request - execute the script
                        return executeCgiScript(file_path, cgi_it->second, request, file_path);
                    } else {
                        // It's a regular file - serve the actual file content
                        std::string detected_mime = getMimeType(sanitized_uri);
                        
                        // Read the file content
                        std::ifstream file(file_path.c_str(), std::ios::binary);
                        if (file.is_open()) {
                            std::string file_content((std::istreambuf_iterator<char>(file)),
                                                    std::istreambuf_iterator<char>());
                            file.close();
                            return HttpResponse::createOkResponse(file_content, detected_mime);
                        } else {
                            // File exists but can't be read - return 403 Forbidden
                            return HttpResponse::createForbiddenResponse();
                        }
                    }
                }
            } else {
                // Path does not exist, return custom 404
                std::ifstream error_file("./www/404.html", std::ios::binary);
                if (error_file.is_open()) {
                    std::string file_content((std::istreambuf_iterator<char>(error_file)),
                                            std::istreambuf_iterator<char>());
                    error_file.close();
                    HttpResponse response;
                    response.setStatusCode(404);
                    response.setContentType("text/html");
                    response.setBody(file_content);
                    response.setConnection(false);
                    return response;
                } else {
                    return createErrorResponse(404);
                }
            }
    } else if (method == "POST") {
        // Check if this is a CGI request
        std::string file_extension;
        size_t dot_pos = sanitized_uri.find_last_of('.');
        if (dot_pos != std::string::npos) {
            file_extension = sanitized_uri.substr(dot_pos);
        }
        
        // Get CGI extensions from location
        const std::map<std::string, std::string>& cgi_extensions = location->getCgiExtensions();
        std::map<std::string, std::string>::const_iterator cgi_it = cgi_extensions.find(file_extension);
        
        if (cgi_it != cgi_extensions.end()) {
            // This is a CGI request - execute the script
            std::string file_path = location->getRoot();
            if (file_path.empty() || file_path[file_path.length() - 1] != '/') {
                file_path += "/";
            }
            
            // Nginx-style path construction: simply concatenate root + URI
            file_path += sanitized_uri;
            
            // Check if the CGI script file exists
            if (access(file_path.c_str(), F_OK) == 0) {
                // Execute CGI script
                return executeCgiScript(file_path, cgi_it->second, request, file_path);
            } else {
                return createErrorResponse(404);
            }
        } else {
            // Check if this is a file upload request
            std::string upload_path = location->getUploadPath();
            if (!upload_path.empty()) {
                // This is an upload location - handle file upload
                return handleFileUpload(request, location, sanitized_uri);
            } else {
                // Regular POST request (not CGI)
                std::string body = "POST request received\nURI: " + sanitized_uri + "\nBody: " + request.getBody();
                return HttpResponse::createOkResponse(body, "text/plain");
            }
        }
    } else if (method == "DELETE") {
        std::string body = "DELETE request received\nURI: " + sanitized_uri;
        return HttpResponse::createOkResponse(body, "text/plain");
    } else if (method == "PUT") {
        // Handle PUT request - save file to upload path
        std::string upload_path = location->getUploadPath();
        if (upload_path.empty()) {
            return HttpResponse::createServerErrorResponse();
        }
        
        // Create upload directory if it doesn't exist
        struct stat st;
        if (stat(upload_path.c_str(), &st) != 0) {
            // Directory doesn't exist, try to create it
            if (mkdir(upload_path.c_str(), 0755) != 0) {
                return HttpResponse::createServerErrorResponse();
            }
        }
        
        // Generate filename or use URI path
        std::ostringstream oss;
        oss << "uploaded_file_" << time(NULL);
        std::string filename = oss.str();
        std::string full_path = upload_path + "/" + filename;
        
        // Write body content to file
        std::ofstream file(full_path.c_str(), std::ios::binary);
        if (!file.is_open()) {
            return HttpResponse::createServerErrorResponse();
        }
        
        const std::string& body = request.getBody();
        if (!body.empty()) {
            file.write(body.c_str(), body.size());
        }
        file.close();
        
        // Return success response
        std::string response_body = "PUT request successful\nFile saved to: " + full_path;
        return HttpResponse::createOkResponse(response_body, "text/plain");
    }
    
    return HttpResponse::createServerErrorResponse();
}

/*
 * Handles reading data from a client socket
 * Receives data, processes it, or handles connection errors/disconnects
 * Uses non-blocking I/O
 */
void ConnectionHandler::handleClientRead(int client_sock) {
    char buffer[65536];  // Increased to 64KB to handle larger bodies
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
        // recv() returned -1, remove client without checking errno
        std::cerr << "Error reading from client " << client_sock << std::endl;
        removeClient(client_sock);
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
            
            if (client.isKeepAlive()) {
                // Keep connection alive - reset buffers for next request
                std::cout << "Keeping connection alive for client " << client_sock << std::endl;
                client.clearReadBuffer();
                client.clearWriteBuffer();
                client.setBytesSent(0);
                client.setKeepAlive(false); // Reset for next request
            } else {
                removeClient(client_sock);
            }
        }
    } else if (bytes_sent == 0) {
        std::cout << "Client " << client_sock << " closed connection during write" << std::endl;
        removeClient(client_sock);
    } else {
        // send() returned -1, remove client without checking errno
        std::cerr << "Error writing to client " << client_sock << std::endl;
        removeClient(client_sock);
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

/*
 * Returns the current server configuration based on the client socket
 * Uses getsockname() to determine which server port the client connected to
 */
const ServerConfig* ConnectionHandler::getCurrentServerConfig(int client_sock) const {
    if (!_server_configs || _server_configs->empty()) {
        return NULL;
    }
    
    // Get the local address/port of the connected socket
    struct sockaddr_in local_addr;
    socklen_t addr_len = sizeof(local_addr);
    if (getsockname(client_sock, (struct sockaddr*)&local_addr, &addr_len) < 0) {
        // If we can't determine the port, default to first server config
        return &(*_server_configs)[0];
    }
    
    int local_port = ntohs(local_addr.sin_port);
    
    // Find the server config that matches this port
    for (size_t i = 0; i < _server_configs->size(); ++i) {
        if ((*_server_configs)[i].getPort() == local_port) {
            return &(*_server_configs)[i];
        }
    }
    
    // If no match found, default to first server config
    return &(*_server_configs)[0];
}

/*
 * Executes a CGI script and returns the HTTP response
 * Handles fork/exec, environment setup, and I/O communication
 */
HttpResponse ConnectionHandler::executeCgiScript(const std::string& script_path, 
                                                 const std::string& interpreter_path,
                                                 const HttpRequest& request, 
                                                 const std::string& file_path) const {
    int pipefd_in[2];   // for sending data to CGI
    int pipefd_out[2];  // for receiving data from CGI
    
    // Create pipes for communication with CGI process
    if (pipe(pipefd_in) == -1 || pipe(pipefd_out) == -1) {
        return HttpResponse::createServerErrorResponse();
    }
    
    pid_t pid = fork();
    if (pid == -1) {
        close(pipefd_in[0]);
        close(pipefd_in[1]);
        close(pipefd_out[0]);
        close(pipefd_out[1]);
        return HttpResponse::createServerErrorResponse();
    }
    
    if (pid == 0) {
        // Child process - execute CGI script
        
        // Set up pipes: stdin from parent, stdout to parent
        dup2(pipefd_in[0], STDIN_FILENO);
        dup2(pipefd_out[1], STDOUT_FILENO);
        
        // Close all pipe file descriptors
        close(pipefd_in[0]);
        close(pipefd_in[1]);
        close(pipefd_out[0]);
        close(pipefd_out[1]);
        
        // Set environment variables for CGI
        setenv("REQUEST_METHOD", request.getMethod().c_str(), 1);
        setenv("CONTENT_TYPE", request.getHeader("Content-Type").c_str(), 1);
        setenv("CONTENT_LENGTH", request.getHeader("Content-Length").c_str(), 1);
        setenv("SCRIPT_NAME", script_path.c_str(), 1);
        setenv("PATH_INFO", file_path.c_str(), 1);
        setenv("QUERY_STRING", "", 1);  // No query string for now
        setenv("SERVER_PROTOCOL", "HTTP/1.1", 1);
        setenv("GATEWAY_INTERFACE", "CGI/1.1", 1);
        
        // Execute the CGI script
        char* args[] = {const_cast<char*>(interpreter_path.c_str()), const_cast<char*>(script_path.c_str()), NULL};
        execve(interpreter_path.c_str(), args, environ);
        
        // If execve fails, exit
        std::exit(1);
    } else {
        // Parent process - communicate with CGI
        
        // Close unused pipe ends
        close(pipefd_in[0]);
        close(pipefd_out[1]);
        
        // Send request body to CGI if present
        const std::string& body = request.getBody();
        if (!body.empty()) {
            write(pipefd_in[1], body.c_str(), body.length());
        }
        close(pipefd_in[1]);  // Signal end of input
        
        // Read CGI output
        std::string cgi_output;
        char buffer[1024];
        ssize_t bytes_read;
        while ((bytes_read = read(pipefd_out[0], buffer, sizeof(buffer))) > 0) {
            cgi_output.append(buffer, bytes_read);
        }
        close(pipefd_out[0]);
        
        // Wait for CGI process to finish
        int status;
        waitpid(pid, &status, 0);
        
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            // CGI executed successfully
            // Parse CGI output to separate headers and body
            size_t header_end = cgi_output.find("\r\n\r\n");
            if (header_end == std::string::npos) {
                header_end = cgi_output.find("\n\n");
                if (header_end != std::string::npos) {
                    header_end += 2;
                } else {
                    header_end = 0;
                }
            } else {
                header_end += 4;
            }
            
            std::string cgi_body;
            if (header_end < cgi_output.length()) {
                cgi_body = cgi_output.substr(header_end);
            } else {
                cgi_body = cgi_output;
            }
            
            // Create response with CGI output
            HttpResponse response;
            response.setStatusCode(200);
            response.setContentType("text/html");
            response.setBody(cgi_body);
            response.setConnection(false);
            return response;
        } else {
            // CGI execution failed
            return HttpResponse::createServerErrorResponse();
        }
    }
}

/*
 * Handle file upload for POST requests with multipart/form-data
 */
HttpResponse ConnectionHandler::handleFileUpload(const HttpRequest& request, const Location* location, const std::string& /* uri */) {
    std::string upload_path = location->getUploadPath();
    if (upload_path.empty()) {
        return createErrorResponse(500);
    }
    
    // Create upload directory if it doesn't exist
    struct stat st;
    if (stat(upload_path.c_str(), &st) != 0) {
        if (mkdir(upload_path.c_str(), 0755) != 0) {
            return createErrorResponse(500);
        }
    }
    
    std::string body = request.getBody();
    if (body.empty()) {
        return createErrorResponse(400);
    }
    
    std::string filename;
    std::string file_content;
    
    // Check if this is multipart/form-data
    std::string content_type = request.getHeader("Content-Type");
    if (content_type.find("multipart/form-data") != std::string::npos) {
        // Extract boundary from Content-Type header
        size_t boundary_pos = content_type.find("boundary=");
        if (boundary_pos == std::string::npos) {
            return createErrorResponse(400);
        }
        
        std::string boundary = "--" + content_type.substr(boundary_pos + 9);
        
        // Simple multipart parsing - find file data
        size_t start_pos = body.find(boundary);
        if (start_pos == std::string::npos) {
            return createErrorResponse(400);
        }
        
        // Find the file content between boundaries
        size_t content_start = body.find("\r\n\r\n", start_pos);
        if (content_start == std::string::npos) {
            return createErrorResponse(400);
        }
        content_start += 4; // Skip \r\n\r\n
        
        size_t content_end = body.find(boundary, content_start);
        if (content_end == std::string::npos) {
            return createErrorResponse(400);
        }
        content_end -= 2; // Remove \r\n before boundary
        
        file_content = body.substr(content_start, content_end - content_start);
        
        // Extract filename from Content-Disposition header if present
        size_t filename_pos = body.find("filename=\"", start_pos);
        if (filename_pos != std::string::npos && filename_pos < content_start) {
            filename_pos += 10; // Skip filename="
            size_t filename_end = body.find("\"", filename_pos);
            if (filename_end != std::string::npos) {
                filename = body.substr(filename_pos, filename_end - filename_pos);
            }
        }
    } else {
        // Regular POST body
        file_content = body;
    }
    
    // Generate filename if not extracted from multipart
    if (filename.empty()) {
        std::ostringstream oss;
        oss << "upload_" << time(NULL) << ".bin";
        filename = oss.str();
    }
    
    std::string full_path = upload_path + "/" + filename;
    
    // Write file content
    std::ofstream file(full_path.c_str(), std::ios::binary);
    if (!file.is_open()) {
        return createErrorResponse(500);
    }
    
    file.write(file_content.c_str(), file_content.length());
    file.close();
    
    // Return success response with redirect to upload directory
    std::string response_body = "<!DOCTYPE html><html><head><title>Upload Success</title></head><body>";
    response_body += "<h1>File Upload Successful</h1>";
    response_body += "<p>File saved as: " + filename + "</p>";
    std::ostringstream size_stream;
    size_stream << file_content.length();
    response_body += "<p>Size: " + size_stream.str() + " bytes</p>";
    response_body += "<p><a href=\"/upload/\">View Uploaded Files</a></p>";
    response_body += "<p><a href=\"/\">Back to Home</a></p>";
    response_body += "</body></html>";
    
    return HttpResponse::createOkResponse(response_body, "text/html");
}

/*
 * Creates an error response, checking for custom error pages in common locations
 * Falls back to default error response if no custom page is found
 */
HttpResponse ConnectionHandler::createErrorResponse(int error_code) const {
    // Try to get custom error page from server configuration
    if (_server_configs && !_server_configs->empty()) {
        const ServerConfig& config = (*_server_configs)[0];
        const std::map<int, std::string>& error_pages = config.getErrorPages();
        
        std::map<int, std::string>::const_iterator it = error_pages.find(error_code);
        if (it != error_pages.end()) {
            std::string error_page_path = it->second;
            
            // Try to load the custom error page
            std::ifstream error_file(error_page_path.c_str(), std::ios::binary);
            if (error_file.is_open()) {
                std::string file_content((std::istreambuf_iterator<char>(error_file)),
                                        std::istreambuf_iterator<char>());
                error_file.close();
                
                HttpResponse response;
                response.setStatusCode(error_code);
                response.setContentType("text/html");
                response.setBody(file_content);
                response.setConnection(false);
                return response;
            }
        }
    }
    
    // Fallback to default error responses
    if (error_code == 404) {
        return HttpResponse::createNotFoundResponse();
    } else if (error_code == 500) {
        return HttpResponse::createServerErrorResponse();
    } else if (error_code == 403) {
        return HttpResponse::createForbiddenResponse();
    }
    return HttpResponse::createServerErrorResponse();
}

