#include "ConfigParser.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cctype>
#include <cstdlib>

/*
 * Default constructor for ConfigParser
 * Initializes the parser with empty state
 */
ConfigParser::ConfigParser() {}

/*
 * Destructor for ConfigParser
 * Cleans up any allocated resources
 */
ConfigParser::~ConfigParser() {}

/*
 * Checks if the parser has encountered any errors during parsing
 * Returns true if errors exist, false otherwise
 */
bool ConfigParser::hasErrors() const {
    return _validator.hasErrors();
}

/*
 * Validates that the current token matches the expected token
 * Advances the tokenizer if the token matches
 * Returns true if token matches, false otherwise
 */
bool ConfigParser::expectToken(const std::string& expected) {
    if (!hasNextToken()) {
        _validator.validateToken(expected, "");
        return false;
    }
    
    if (!_validator.validateToken(expected, getCurrentToken())) {
        return false;
    }
    
    skipToken();
    
    if (expected == ";" && checkForMultipleSemicolons()) {
        return false;
    }
    
    return true;
}

/*
 * Checks for multiple consecutive semicolons in the token stream
 * Returns true if multiple semicolons are found, false otherwise
 */
bool ConfigParser::checkForMultipleSemicolons() {
    if (hasNextToken()) {
        return !_validator.checkMultipleSemicolons(";", getCurrentToken());
    }
    return false;
}

/*
 * Returns the current token from the tokenizer
 * Does not advance the tokenizer position
 */
std::string ConfigParser::getCurrentToken() {
    return _tokenizer.getCurrentToken();
}

/*
 * Returns the next token and advances the tokenizer position
 * Used to consume tokens during parsing
 */
std::string ConfigParser::getNextToken() {
    return _tokenizer.getNextToken();
}

/*
 * Checks if there are more tokens available for parsing
 * Returns true if more tokens exist, false otherwise
 */
bool ConfigParser::hasNextToken() {
    return _tokenizer.hasNextToken();
}

/*
 * Skips the current token and advances to the next one
 * Used to consume tokens without returning their value
 */
void ConfigParser::skipToken() {
    _tokenizer.skipToken();
}

/*
 * Skips any extra semicolons in the token stream
 * Continues until a non-semicolon token is found
 */
void ConfigParser::skipExtraSemicolons() {
    while (hasNextToken() && getCurrentToken() == ";") {
        skipToken();
    }
}

/*
 * Skips tokens until the end of the current block
 * Uses brace counting to handle nested blocks correctly
 * Assumes we're already inside a block when called
 */
void ConfigParser::skipToEndOfBlock() {
    int brace_count = 1; // Assume we're already inside a block
    
    // Skip until we find the matching closing brace
    while (hasNextToken() && brace_count > 0) {
        std::string token = getNextToken();
        if (token == "{") {
            brace_count++;
        } else if (token == "}") {
            brace_count--;
        }
    }
}

/*
 * Parses a list of string values from the token stream
 * Continues until a semicolon or closing brace is found
 * Validates that directive keywords are not mixed with values
 * Returns vector of parsed string values
 */
std::vector<std::string> ConfigParser::parseStringList() {
    std::vector<std::string> result;
    
    while (hasNextToken() && getCurrentToken() != ";" && getCurrentToken() != "}") {
        std::string token = getCurrentToken();
        
        if (token == "root" || token == "autoindex" || token == "index" || 
            token == "methods" || token == "allow_methods" || token == "upload_path" || 
            token == "cgi_extension" || token == "cgi_extensions" || token == "return" || 
            token == "listen" || token == "server_name" || token == "error_page" || 
            token == "client_max_body_size" || token == "location") {
            std::cerr << "Error: Expected ';' after directive but found directive '" << token << "'" << std::endl;
            _validator.addError("Expected ';' after directive but found directive '" + token + "'");
            return result;
        }
        
        result.push_back(getNextToken());
    }
    
    if (hasNextToken() && getCurrentToken() == "}") {
        std::cerr << "Error: Expected ';' after directive but found '}'" << std::endl;
        _validator.addError("Expected ';' after directive but found '}'");
        return result;
    }
    
    if (!expectToken(";")) {
        _validator.addError("Expected ';' after directive");
        return result;
    }
    
    return result;
}

/*
 * Parses a list of HTTP methods from the token stream
 * Validates each method using the validator
 * Returns vector of valid HTTP methods (GET, POST, DELETE)
 */
std::vector<std::string> ConfigParser::parseHttpMethods() {
    std::vector<std::string> result;
    
    while (hasNextToken() && getCurrentToken() != ";" && getCurrentToken() != "}") {
        std::string token = getCurrentToken();
        
        if (token == "root" || token == "autoindex" || token == "index" || 
            token == "methods" || token == "allow_methods" || token == "upload_path" || 
            token == "cgi_extension" || token == "cgi_extensions" || token == "return" || 
            token == "listen" || token == "server_name" || token == "error_page" || 
            token == "client_max_body_size" || token == "location") {
            std::cerr << "Error: Expected ';' after directive but found directive '" << token << "'" << std::endl;
            _validator.addError("Expected ';' after directive but found directive '" + token + "'");
            return result;
        }
        
        // Validate HTTP method
        if (!_validator.validateHttpMethod(token)) {
            _validator.addError("Invalid HTTP method: " + token);
            return result;
        }
        
        result.push_back(getNextToken());
    }
    
    if (hasNextToken() && getCurrentToken() == "}") {
        std::cerr << "Error: Expected ';' after directive but found '}'" << std::endl;
        _validator.addError("Expected ';' after directive but found '}'");
        return result;
    }
    
    if (!expectToken(";")) {
        _validator.addError("Expected ';' after methods directive");
        return result;
    }
    
    return result;
}

/*
 * Parses a location block from the configuration file
 * Handles location path and all location-specific directives
 * Returns a configured Location object
 */
Location ConfigParser::parseLocationBlock() {
    Location location;
    
    // Parse location path
    if (hasNextToken()) {
        location.setPath(getNextToken());
    }
    
    // Expect opening brace
    if (!hasNextToken() || getCurrentToken() != "{") {
        std::cerr << "Error: Expected '{' after location path" << std::endl;
        _validator.addError("Expected '{' after location path");
        return location;
    }
    skipToken();
    
    // Track singleton directives to detect duplicates
    bool root_found = false;
    bool upload_path_found = false;
    bool return_found = false;
    bool autoindex_found = false;
    
    // Parse location directives
    while (hasNextToken() && getCurrentToken() != "}") {
        std::string directive = getNextToken();
        
        if (directive == "allow_methods" || directive == "methods") {
            std::vector<std::string> methods = parseHttpMethods();
            // Check for errors after parsing methods
            if (_validator.hasErrors()) {
                return location;
            }
            location.setMethods(methods);
        } else if (directive == "root") {
            // Check for duplicate root directive
            if (root_found) {
                std::cerr << "Error: Duplicate 'root' directive found in location block" << std::endl;
                _validator.addError("Duplicate 'root' directive found in location block");
                return location;
            }
            root_found = true;
            if (hasNextToken()) {
                location.setRoot(getNextToken());
            } else {
                std::cerr << "Error: Expected value after 'root'" << std::endl;
                _validator.addError("Expected value after 'root'");
                return location;
            }
            if (!hasNextToken() || getCurrentToken() != ";") {
                std::cerr << "Error: Expected ';' after root directive" << std::endl;
                _validator.addError("Expected ';' after root directive");
                return location;
            }
            skipToken();
        } else if (directive == "autoindex") {
            // Check for duplicate autoindex directive
            if (autoindex_found) {
                std::cerr << "Error: Duplicate 'autoindex' directive found in location block" << std::endl;
                _validator.addError("Duplicate 'autoindex' directive found in location block");
                return location;
            }
            autoindex_found = true;
            if (hasNextToken()) {
                std::string value = getNextToken();
                location.setAutoindex(value == "on");
            } else {
                std::cerr << "Error: Expected value after 'autoindex'" << std::endl;
                _validator.addError("Expected value after 'autoindex'");
                return location;
            }
            if (!hasNextToken() || getCurrentToken() != ";") {
                std::cerr << "Error: Expected ';' after autoindex directive" << std::endl;
                _validator.addError("Expected ';' after autoindex directive");
                return location;
            }
            skipToken();
        } else if (directive == "index") {
            std::vector<std::string> indexFiles = parseStringList();
            // Check for errors after parsing string list
            if (_validator.hasErrors()) {
                return location;
            }
            location.setIndexFiles(indexFiles);
        } else if (directive == "upload_path") {
            // Check for duplicate upload_path directive
            if (upload_path_found) {
                std::cerr << "Error: Duplicate 'upload_path' directive found in location block" << std::endl;
                _validator.addError("Duplicate 'upload_path' directive found in location block");
                return location;
            }
            upload_path_found = true;
            if (hasNextToken()) {
                location.setUploadPath(getNextToken());
            } else {
                std::cerr << "Error: Expected value after 'upload_path'" << std::endl;
                _validator.addError("Expected value after 'upload_path'");
                return location;
            }
            if (!hasNextToken() || getCurrentToken() != ";") {
                std::cerr << "Error: Expected ';' after upload_path directive" << std::endl;
                _validator.addError("Expected ';' after upload_path directive");
                return location;
            }
            skipToken();
        } else if (directive == "cgi_extension" || directive == "cgi_extensions") {
            // Handle both single extension+path and multiple extensions
            std::vector<std::string> tokens;
            while (hasNextToken() && getCurrentToken() != ";" && getCurrentToken() != "}") {
                std::string token = getCurrentToken();
                
                // Check if we hit another directive instead of semicolon
                if (token == "root" || token == "autoindex" || token == "index" || 
                    token == "methods" || token == "allow_methods" || token == "upload_path" || 
                    token == "cgi_extension" || token == "cgi_extensions" || token == "return" || 
                    token == "listen" || token == "server_name" || token == "error_page" || 
                    token == "client_max_body_size" || token == "location") {
                    std::cerr << "Error: Expected ';' after directive but found directive '" << token << "'" << std::endl;
                    _validator.addError("Expected ';' after directive but found directive '" + token + "'");
                    return location;
                }
                
                tokens.push_back(getNextToken());
            }
            
            if (hasNextToken() && getCurrentToken() == "}") {
                std::cerr << "Error: Expected ';' after directive but found '}'" << std::endl;
                _validator.addError("Expected ';' after directive but found '}'");
                return location;
            }
            
            if (!expectToken(";")) {
                _validator.addError("Expected ';' after cgi_extension directive");
                return location;
            }
            
            // CGI extensions must be provided as extension-path pairs
            // Only .py and .php extensions are supported
            if (tokens.size() % 2 != 0) {
                std::cerr << "Error: cgi_extension(s) requires pairs of extension and path" << std::endl;
                _validator.addError("cgi_extension(s) requires pairs of extension and path");
                return location;
            }
            
            for (size_t i = 0; i < tokens.size(); i += 2) {
                std::string extension = tokens[i];
                std::string path = tokens[i + 1];
                
                // Validate that only .py, .php, and .bla extensions are supported
                if (extension != ".py" && extension != ".php" && extension != ".bla") {
                    std::cerr << "Error: Unsupported CGI extension '" << extension << "'. Only .py, .php, and .bla are supported." << std::endl;
                    _validator.addError("Unsupported CGI extension '" + extension + "'. Only .py, .php, and .bla are supported.");
                    return location;
                }
                
                // Validate that path is provided (not empty and starts with /)
                if (path.empty() || path[0] != '/') {
                    std::cerr << "Error: CGI path must be an absolute path starting with '/'" << std::endl;
                    _validator.addError("CGI path must be an absolute path starting with '/'");
                    return location;
                }
                
                location.addCgiExtension(extension, path);
            }
        } else if (directive == "return") {
            // Check for duplicate return directive
            if (return_found) {
                std::cerr << "Error: Duplicate 'return' directive found in location block" << std::endl;
                _validator.addError("Duplicate 'return' directive found in location block");
                return location;
            }
            return_found = true;
            std::string redirect_value;
            if (hasNextToken()) {
                std::string first_token = getNextToken();
                
                // Check if first token is a status code (3xx)
                if (first_token.length() == 3 && 
                    first_token[0] == '3' && 
                    std::isdigit(first_token[1]) && 
                    std::isdigit(first_token[2])) {
                    
                    // This is a status code, expect a URL next
                    if (hasNextToken()) {
                        std::string url = getNextToken();
                        redirect_value = first_token + " " + url;
                    } else {
                        std::cerr << "Error: Expected URL after return status code" << std::endl;
                        _validator.addError("Expected URL after return status code");
                        return location;
                    }
                } else {
                    // This is just a URL without status code
                    redirect_value = first_token;
                }
                
                location.setRedirect(redirect_value);
            } else {
                std::cerr << "Error: Expected value after 'return'" << std::endl;
                _validator.addError("Expected value after 'return'");
                return location;
            }
            if (!expectToken(";")) {
                _validator.addError("Expected ';' after return directive");
                return location;
            }
        } else if (directive == ";") {
            _validator.validateDirective(directive, "location");
            return location;
        } 
        else
        {
            _validator.validateDirective(directive, "location");
            return location;
        }
    }
    
    if (!hasNextToken() || getCurrentToken() != "}") {
        std::cerr << "Error: Expected '}' at end of location block" << std::endl;
        _validator.addError("Expected '}' at end of location block");
        return location;
    }
    skipToken();
    
    return location;
}

/*
 * Parses a server block from the configuration file
 * Handles all server-level directives (listen, server_name, error_page, etc.)
 * Returns a configured ServerConfig object
 */
ServerConfig ConfigParser::parseServerBlock() {
    ServerConfig config;
    bool listen_directive_found = false;
    bool client_max_body_size_found = false;
    
    // Expect opening brace
    if (!hasNextToken() || getCurrentToken() != "{") {
        std::cerr << "Error: Expected '{' after 'server'" << std::endl;
        _validator.addError("Expected '{' after 'server'");
        return config;
    }
    skipToken();
    
    // Parse server directives
    while (hasNextToken() && getCurrentToken() != "}") {
        std::string directive = getNextToken();
        
        if (directive == "listen") {
            // Check for duplicate listen directive
            if (listen_directive_found) {
                std::cerr << "Error: Duplicate 'listen' directive found in server block" << std::endl;
                _validator.addError("Duplicate 'listen' directive found in server block");
                return config;
            }
            listen_directive_found = true;
            if (hasNextToken()) {
                std::string listen_value = getNextToken();
                size_t colon_pos = listen_value.find(':');
                if (colon_pos != std::string::npos) {
                    config.setHost(listen_value.substr(0, colon_pos));
                    config.setPort(std::atoi(listen_value.substr(colon_pos + 1).c_str()));
                } else {
                    config.setPort(std::atoi(listen_value.c_str()));
                }
            } else {
                std::cerr << "Error: Expected value after 'listen'" << std::endl;
                _validator.addError("Expected value after 'listen'");
                return config;
            }
            if (!expectToken(";")) {
                _validator.addError("Expected ';' after listen directive");
                return config;
            }
        } else if (directive == "server_name") {
            std::vector<std::string> serverNames = parseStringList();
            // Check for errors after parsing server names
            if (_validator.hasErrors()) {
                return config;
            }
            config.setServerNames(serverNames);
        } else if (directive == "error_page") {
            if (hasNextToken()) {
                int error_code = std::atoi(getNextToken().c_str());
                if (hasNextToken()) {
                    config.addErrorPage(error_code, getNextToken());
                } else {
                    std::cerr << "Error: Expected page path after error code" << std::endl;
                    _validator.addError("Expected page path after error code");
                    return config;
                }
            } else {
                std::cerr << "Error: Expected error code after 'error_page'" << std::endl;
                _validator.addError("Expected error code after 'error_page'");
                return config;
            }
            if (!hasNextToken() || getCurrentToken() != ";") {
                std::cerr << "Error: Expected ';' after error_page directive" << std::endl;
                _validator.addError("Expected ';' after error_page directive");
                return config;
            }
            skipToken();
        } else if (directive == "client_max_body_size") {
            // Check for duplicate client_max_body_size directive
            if (client_max_body_size_found) {
                std::cerr << "Error: Duplicate 'client_max_body_size' directive found in server block" << std::endl;
                _validator.addError("Duplicate 'client_max_body_size' directive found in server block");
                return config;
            }
            client_max_body_size_found = true;
            if (hasNextToken()) {
                std::string size_str = getNextToken();
                size_t max_body_size = std::atoi(size_str.c_str());
                // Handle size suffixes (k, m, g)
                if (!size_str.empty()) {
                    char suffix = size_str[size_str.length() - 1];
                    if (suffix == 'k' || suffix == 'K') {
                        max_body_size *= 1024;
                    } else if (suffix == 'm' || suffix == 'M') {
                        max_body_size *= 1024 * 1024;
                    } else if (suffix == 'g' || suffix == 'G') {
                        max_body_size *= 1024 * 1024 * 1024;
                    }
                }
                config.setMaxBodySize(max_body_size);
            } else {
                std::cerr << "Error: Expected size value after 'client_max_body_size'" << std::endl;
                _validator.addError("Expected size value after 'client_max_body_size'");
                return config;
            }
            if (!hasNextToken() || getCurrentToken() != ";") {
                std::cerr << "Error: Expected ';' after client_max_body_size directive" << std::endl;
                _validator.addError("Expected ';' after client_max_body_size directive");
                return config;
            }
            skipToken();
        } else if (directive == "location") {
            Location loc = parseLocationBlock();
            // Check for errors after parsing location block
            if (_validator.hasErrors()) {
                return config;
            }
            config.addLocation(loc);
        } else if (directive == ";") {
            _validator.validateDirective(directive, "server");
            return config;
        } else {
            _validator.validateDirective(directive, "server");
            return config;
        }
    }
    
    if (!hasNextToken() || getCurrentToken() != "}") {
        std::cerr << "Error: Expected '}' at end of server block" << std::endl;
        _validator.addError("Expected '}' at end of server block");
        return config;
    }
    skipToken();
    
    return config;
}

/*
 * Main parsing function that reads and parses the configuration file
 * Opens the file, tokenizes its content, and parses server blocks
 * Returns a vector of ServerConfig objects for all valid server blocks
 */
std::vector<ServerConfig> ConfigParser::parse(const std::string& config_file) {
    std::vector<ServerConfig> servers;
    std::ifstream file(config_file.c_str());
    
    if (!file.is_open()) {
        std::cerr << "Error: Could not open config file: " << config_file << std::endl;
        _validator.addError("Could not open config file: " + config_file);
        return servers;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    file.close();
    
    _tokenizer.tokenize(content);
    _validator.resetErrors();
    
    while (hasNextToken()) {
        std::string token = getNextToken();
        if (token == "server") {
            ServerConfig config = parseServerBlock();
            
            // Only add the server if there are no errors
            if (!_validator.hasErrors()) {
                servers.push_back(config);
            } else {
                std::cerr << "Error: Failed to parse server block due to syntax errors. Server will not start." << std::endl;
                // Return empty servers list to signal complete failure
                servers.clear();
                return servers;
            }
        } else {
            std::cerr << "Error: Unknown top-level directive '" << token << "'" << std::endl;
            _validator.addError("Unknown top-level directive '" + token + "'");
            servers.clear();
            return servers;
        }
    }
    
    return servers;
}