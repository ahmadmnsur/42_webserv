#include "ConfigParser.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cctype>
#include <cstdlib>

ConfigParser::ConfigParser() : _current_token(0), _has_errors(false) {}

ConfigParser::~ConfigParser() {}

bool ConfigParser::hasErrors() const {
    return _has_errors;
}

bool ConfigParser::expectToken(const std::string& expected) {
    if (!hasNextToken()) {
        std::cerr << "Error: Expected '" << expected << "' but reached end of file" << std::endl;
        _has_errors = true;
        return false;
    }
    
    if (getCurrentToken() != expected) {
        std::cerr << "Error: Expected '" << expected << "' but found '" << getCurrentToken() << "'" << std::endl;
        _has_errors = true;
        return false;
    }
    
    skipToken();
    return true;
}

void ConfigParser::tokenize(const std::string& content) {
    _tokens.clear();
    std::string current_token;
    bool in_string = false;
    
    for (size_t i = 0; i < content.length(); ++i) {
        char c = content[i];
        
        if (c == '"' && !in_string) {
            in_string = true;
            continue;
        } else if (c == '"' && in_string) {
            in_string = false;
            if (!current_token.empty()) {
                _tokens.push_back(current_token);
                current_token.clear();
            }
            continue;
        }
        
        if (in_string) {
            current_token += c;
            continue;
        }
        
        if (std::isspace(c) || c == '#') {
            if (!current_token.empty()) {
                _tokens.push_back(current_token);
                current_token.clear();
            }
            if (c == '#') {
                // Skip comment until end of line
                while (i < content.length() && content[i] != '\n') {
                    ++i;
                }
            }
        } else if (c == '{' || c == '}' || c == ';') {
            if (!current_token.empty()) {
                _tokens.push_back(current_token);
                current_token.clear();
            }
            _tokens.push_back(std::string(1, c));
        } else {
            current_token += c;
        }
    }
    
    if (!current_token.empty()) {
        _tokens.push_back(current_token);
    }
}

std::string ConfigParser::getCurrentToken() {
    if (_current_token < _tokens.size()) {
        return _tokens[_current_token];
    }
    return "";
}

std::string ConfigParser::getNextToken() {
    if (_current_token < _tokens.size()) {
        return _tokens[_current_token++];
    }
    return "";
}

bool ConfigParser::hasNextToken() {
    return _current_token < _tokens.size();
}

void ConfigParser::skipToken() {
    if (_current_token < _tokens.size()) {
        _current_token++;
    }
}

std::vector<std::string> ConfigParser::parseStringList() {
    std::vector<std::string> result;
    
    while (hasNextToken() && getCurrentToken() != ";" && getCurrentToken() != "}") {
        std::string token = getCurrentToken();
        
        // Check if we hit another directive instead of semicolon
        if (token == "root" || token == "autoindex" || token == "index" || 
            token == "methods" || token == "allow_methods" || token == "upload_path" || 
            token == "cgi_extension" || token == "return" || token == "listen" || 
            token == "server_name" || token == "error_page" || token == "client_max_body_size" ||
            token == "location") {
            std::cerr << "Error: Expected ';' after directive but found directive '" << token << "'" << std::endl;
            _has_errors = true;
            return result;
        }
        
        result.push_back(getNextToken());
    }
    
    // Check if we stopped because we hit a '}' instead of ';'
    if (hasNextToken() && getCurrentToken() == "}") {
        std::cerr << "Error: Expected ';' after directive but found '}'" << std::endl;
        _has_errors = true;
        return result;
    }
    
    if (!expectToken(";")) {
        _has_errors = true;
        return result;
    }
    
    return result;
}

Location ConfigParser::parseLocationBlock() {
    Location location;
    
    // Parse location path
    if (hasNextToken()) {
        location.setPath(getNextToken());
    }
    
    // Expect opening brace
    if (!hasNextToken() || getCurrentToken() != "{") {
        std::cerr << "Error: Expected '{' after location path" << std::endl;
        return location;
    }
    skipToken();
    
    // Parse location directives
    while (hasNextToken() && getCurrentToken() != "}") {
        std::string directive = getNextToken();
        
        if (directive == "allow_methods" || directive == "methods") {
            location.setMethods(parseStringList());
        } else if (directive == "root") {
            if (hasNextToken()) {
                location.setRoot(getNextToken());
            } else {
                std::cerr << "Error: Expected value after 'root'" << std::endl;
                return location;
            }
            if (!hasNextToken() || getCurrentToken() != ";") {
                std::cerr << "Error: Expected ';' after root directive" << std::endl;
                return location;
            }
            skipToken();
        } else if (directive == "autoindex") {
            if (hasNextToken()) {
                std::string value = getNextToken();
                location.setAutoindex(value == "on");
            } else {
                std::cerr << "Error: Expected value after 'autoindex'" << std::endl;
                return location;
            }
            if (!hasNextToken() || getCurrentToken() != ";") {
                std::cerr << "Error: Expected ';' after autoindex directive" << std::endl;
                return location;
            }
            skipToken();
        } else if (directive == "index") {
            location.setIndexFiles(parseStringList());
        } else if (directive == "upload_path") {
            if (hasNextToken()) {
                location.setUploadPath(getNextToken());
            } else {
                std::cerr << "Error: Expected value after 'upload_path'" << std::endl;
                return location;
            }
            if (!hasNextToken() || getCurrentToken() != ";") {
                std::cerr << "Error: Expected ';' after upload_path directive" << std::endl;
                return location;
            }
            skipToken();
        } else if (directive == "cgi_extension") {
            if (hasNextToken()) {
                std::string ext = getNextToken();
                if (hasNextToken()) {
                    std::string path = getNextToken();
                    location.addCgiExtension(ext, path);
                } else {
                    std::cerr << "Error: Expected path after cgi_extension" << std::endl;
                    return location;
                }
            } else {
                std::cerr << "Error: Expected extension after 'cgi_extension'" << std::endl;
                return location;
            }
            if (!hasNextToken() || getCurrentToken() != ";") {
                std::cerr << "Error: Expected ';' after cgi_extension directive" << std::endl;
                return location;
            }
            skipToken();
        } else if (directive == "return") {
            if (hasNextToken()) {
                location.setRedirect(getNextToken());
            } else {
                std::cerr << "Error: Expected value after 'return'" << std::endl;
                return location;
            }
            if (!hasNextToken() || getCurrentToken() != ";") {
                std::cerr << "Error: Expected ';' after return directive" << std::endl;
                return location;
            }
            skipToken();
        } else {
            std::cerr << "Error: Unknown directive '" << directive << "' in location block" << std::endl;
            // Skip unknown directive until semicolon or end of block
            while (hasNextToken() && getCurrentToken() != ";" && getCurrentToken() != "}") {
                skipToken();
            }
            if (hasNextToken() && getCurrentToken() == ";") {
                skipToken();
            }
        }
    }
    
    // Expect closing brace
    if (!hasNextToken() || getCurrentToken() != "}") {
        std::cerr << "Error: Expected '}' at end of location block" << std::endl;
        return location;
    }
    skipToken();
    
    return location;
}

ServerConfig ConfigParser::parseServerBlock() {
    ServerConfig config;
    
    // Expect opening brace
    if (!hasNextToken() || getCurrentToken() != "{") {
        std::cerr << "Error: Expected '{' after 'server'" << std::endl;
        return config;
    }
    skipToken();
    
    // Parse server directives
    while (hasNextToken() && getCurrentToken() != "}") {
        std::string directive = getNextToken();
        
        if (directive == "listen") {
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
                return config;
            }
            if (!hasNextToken() || getCurrentToken() != ";") {
                std::cerr << "Error: Expected ';' after listen directive" << std::endl;
                return config;
            }
            skipToken();
        } else if (directive == "server_name") {
            config.setServerNames(parseStringList());
        } else if (directive == "error_page") {
            if (hasNextToken()) {
                int error_code = std::atoi(getNextToken().c_str());
                if (hasNextToken()) {
                    config.addErrorPage(error_code, getNextToken());
                } else {
                    std::cerr << "Error: Expected page path after error code" << std::endl;
                    return config;
                }
            } else {
                std::cerr << "Error: Expected error code after 'error_page'" << std::endl;
                return config;
            }
            if (!hasNextToken() || getCurrentToken() != ";") {
                std::cerr << "Error: Expected ';' after error_page directive" << std::endl;
                return config;
            }
            skipToken();
        } else if (directive == "client_max_body_size") {
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
                return config;
            }
            if (!hasNextToken() || getCurrentToken() != ";") {
                std::cerr << "Error: Expected ';' after client_max_body_size directive" << std::endl;
                return config;
            }
            skipToken();
        } else if (directive == "location") {
            config.addLocation(parseLocationBlock());
        } else {
            std::cerr << "Error: Unknown directive '" << directive << "' in server block" << std::endl;
            // Skip unknown directive until semicolon or end of block
            while (hasNextToken() && getCurrentToken() != ";" && getCurrentToken() != "}") {
                skipToken();
            }
            if (hasNextToken() && getCurrentToken() == ";") {
                skipToken();
            }
        }
    }
    
    // Expect closing brace
    if (!hasNextToken() || getCurrentToken() != "}") {
        std::cerr << "Error: Expected '}' at end of server block" << std::endl;
        return config;
    }
    skipToken();
    
    return config;
}

std::vector<ServerConfig> ConfigParser::parse(const std::string& config_file) {
    std::vector<ServerConfig> servers;
    std::ifstream file(config_file.c_str());
    
    if (!file.is_open()) {
        std::cerr << "Error: Could not open config file: " << config_file << std::endl;
        _has_errors = true;
        return servers;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    file.close();
    
    tokenize(content);
    _current_token = 0;
    _has_errors = false;
    
    while (hasNextToken()) {
        std::string token = getNextToken();
        if (token == "server") {
            ServerConfig config = parseServerBlock();
            if (!_has_errors) {
                servers.push_back(config);
            } else {
                std::cerr << "Error: Failed to parse server block, skipping..." << std::endl;
                _has_errors = false; // Reset for next server block
            }
        } else {
            std::cerr << "Error: Unknown top-level directive '" << token << "'" << std::endl;
            _has_errors = true;
            // Skip unknown top-level directive
            while (hasNextToken() && getCurrentToken() != ";" && getCurrentToken() != "{") {
                skipToken();
            }
            if (hasNextToken() && getCurrentToken() == ";") {
                skipToken();
            } else if (hasNextToken() && getCurrentToken() == "{") {
                // Skip block
                int brace_count = 1;
                skipToken();
                while (hasNextToken() && brace_count > 0) {
                    std::string t = getNextToken();
                    if (t == "{") brace_count++;
                    else if (t == "}") brace_count--;
                }
            }
        }
    }
    
    return servers;
}