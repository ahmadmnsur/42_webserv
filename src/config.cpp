// #include "config.hpp"
// #include <cctype>
// #include <cstdlib>

// void ConfigParser::tokenize(const std::string& content) {
//     tokens.clear();
//     std::string current_token;
//     bool in_string = false;
    
//     for (size_t i = 0; i < content.length(); ++i) {
//         char c = content[i];
        
//         if (c == '"' && !in_string) {
//             in_string = true;
//             continue;
//         } else if (c == '"' && in_string) {
//             in_string = false;
//             if (!current_token.empty()) {
//                 tokens.push_back(current_token);
//                 current_token.clear();
//             }
//             continue;
//         }
        
//         if (in_string) {
//             current_token += c;
//             continue;
//         }
        
//         if (std::isspace(c) || c == '#') {
//             if (!current_token.empty()) {
//                 tokens.push_back(current_token);
//                 current_token.clear();
//             }
//             if (c == '#') {
//                 // Skip comment until end of line
//                 while (i < content.length() && content[i] != '\n') {
//                     ++i;
//                 }
//             }
//         } else if (c == '{' || c == '}' || c == ';') {
//             if (!current_token.empty()) {
//                 tokens.push_back(current_token);
//                 current_token.clear();
//             }
//             tokens.push_back(std::string(1, c));
//         } else {
//             current_token += c;
//         }
//     }
    
//     if (!current_token.empty()) {
//         tokens.push_back(current_token);
//     }
// }

// std::string ConfigParser::getCurrentToken() {
//     if (current_token < tokens.size()) {
//         return tokens[current_token];
//     }
//     return "";
// }

// std::string ConfigParser::getNextToken() {
//     if (current_token < tokens.size()) {
//         return tokens[current_token++];
//     }
//     return "";
// }

// bool ConfigParser::hasNextToken() {
//     return current_token < tokens.size();
// }

// void ConfigParser::skipToken() {
//     if (current_token < tokens.size()) {
//         current_token++;
//     }
// }

// std::vector<std::string> ConfigParser::parseStringList() {
//     std::vector<std::string> result;
    
//     while (hasNextToken() && getCurrentToken() != ";" && getCurrentToken() != "}") {
//         result.push_back(getNextToken());
//     }
    
//     if (hasNextToken() && getCurrentToken() == ";") {
//         skipToken(); // Skip semicolon
//     }
    
//     return result;
// }

// Location ConfigParser::parseLocationBlock() {
//     Location location;
    
//     // Parse location path
//     if (hasNextToken()) {
//         location.path = getNextToken();
//     }
    
//     // Expect opening brace
//     if (hasNextToken() && getCurrentToken() == "{") {
//         skipToken();
//     }
    
//     // Parse location directives
//     while (hasNextToken() && getCurrentToken() != "}") {
//         std::string directive = getNextToken();
        
//         if (directive == "allow_methods") {
//             location.methods = parseStringList();
//         } else if (directive == "root") {
//             if (hasNextToken()) {
//                 location.root = getNextToken();
//             }
//             if (hasNextToken() && getCurrentToken() == ";") {
//                 skipToken();
//             }
//         } else if (directive == "autoindex") {
//             if (hasNextToken()) {
//                 std::string value = getNextToken();
//                 location.autoindex = (value == "on");
//             }
//             if (hasNextToken() && getCurrentToken() == ";") {
//                 skipToken();
//             }
//         } else if (directive == "index") {
//             location.index_files = parseStringList();
//         } else if (directive == "upload_path") {
//             if (hasNextToken()) {
//                 location.upload_path = getNextToken();
//             }
//             if (hasNextToken() && getCurrentToken() == ";") {
//                 skipToken();
//             }
//         } else if (directive == "cgi_extension") {
//             if (hasNextToken()) {
//                 std::string ext = getNextToken();
//                 if (hasNextToken()) {
//                     std::string path = getNextToken();
//                     location.cgi_extensions[ext] = path;
//                 }
//             }
//             if (hasNextToken() && getCurrentToken() == ";") {
//                 skipToken();
//             }
//         } else if (directive == "return") {
//             if (hasNextToken()) {
//                 location.redirect = getNextToken();
//             }
//             if (hasNextToken() && getCurrentToken() == ";") {
//                 skipToken();
//             }
//         } else {
//             // Skip unknown directive
//             while (hasNextToken() && getCurrentToken() != ";" && getCurrentToken() != "}") {
//                 skipToken();
//             }
//             if (hasNextToken() && getCurrentToken() == ";") {
//                 skipToken();
//             }
//         }
//     }
    
//     // Skip closing brace
//     if (hasNextToken() && getCurrentToken() == "}") {
//         skipToken();
//     }
    
//     return location;
// }

// ServerConfig ConfigParser::parseServerBlock() {
//     ServerConfig config;
    
//     // Expect opening brace
//     if (hasNextToken() && getCurrentToken() == "{") {
//         skipToken();
//     }
    
//     // Parse server directives
//     while (hasNextToken() && getCurrentToken() != "}") {
//         std::string directive = getNextToken();
        
//         if (directive == "listen") {
//             if (hasNextToken()) {
//                 std::string listen_value = getNextToken();
//                 size_t colon_pos = listen_value.find(':');
//                 if (colon_pos != std::string::npos) {
//                     config.host = listen_value.substr(0, colon_pos);
//                     config.port = std::atoi(listen_value.substr(colon_pos + 1).c_str());
//                 } else {
//                     config.port = std::atoi(listen_value.c_str());
//                 }
//             }
//             if (hasNextToken() && getCurrentToken() == ";") {
//                 skipToken();
//             }
//         } else if (directive == "server_name") {
//             config.server_names = parseStringList();
//         } else if (directive == "error_page") {
//             if (hasNextToken()) {
//                 int error_code = std::atoi(getNextToken().c_str());
//                 if (hasNextToken()) {
//                     config.error_pages[error_code] = getNextToken();
//                 }
//             }
//             if (hasNextToken() && getCurrentToken() == ";") {
//                 skipToken();
//             }
//         } else if (directive == "client_max_body_size") {
//             if (hasNextToken()) {
//                 std::string size_str = getNextToken();
//                 config.max_body_size = std::atoi(size_str.c_str());
//                 // Handle size suffixes (k, m, g)
//                 if (!size_str.empty()) {
//                     char suffix = size_str[size_str.length() - 1];
//                     if (suffix == 'k' || suffix == 'K') {
//                         config.max_body_size *= 1024;
//                     } else if (suffix == 'm' || suffix == 'M') {
//                         config.max_body_size *= 1024 * 1024;
//                     } else if (suffix == 'g' || suffix == 'G') {
//                         config.max_body_size *= 1024 * 1024 * 1024;
//                     }
//                 }
//             }
//             if (hasNextToken() && getCurrentToken() == ";") {
//                 skipToken();
//             }
//         } else if (directive == "location") {
//             config.locations.push_back(parseLocationBlock());
//         } else {
//             // Skip unknown directive
//             while (hasNextToken() && getCurrentToken() != ";" && getCurrentToken() != "}") {
//                 skipToken();
//             }
//             if (hasNextToken() && getCurrentToken() == ";") {
//                 skipToken();
//             }
//         }
//     }
    
//     // Skip closing brace
//     if (hasNextToken() && getCurrentToken() == "}") {
//         skipToken();
//     }
    
//     return config;
// }

// std::vector<ServerConfig> ConfigParser::parse(const std::string& config_file) {
//     std::vector<ServerConfig> servers;
//     std::ifstream file(config_file.c_str());
    
//     if (!file.is_open()) {
//         std::cerr << "Error: Could not open config file: " << config_file << std::endl;
//         return servers;
//     }
    
//     std::stringstream buffer;
//     buffer << file.rdbuf();
//     std::string content = buffer.str();
//     file.close();
    
//     tokenize(content);
//     current_token = 0;
    
//     while (hasNextToken()) {
//         std::string token = getNextToken();
//         if (token == "server") {
//             servers.push_back(parseServerBlock());
//         } else {
//             // Skip unknown top-level directive
//             while (hasNextToken() && getCurrentToken() != ";" && getCurrentToken() != "{") {
//                 skipToken();
//             }
//             if (hasNextToken() && getCurrentToken() == ";") {
//                 skipToken();
//             } else if (hasNextToken() && getCurrentToken() == "{") {
//                 // Skip block
//                 int brace_count = 1;
//                 skipToken();
//                 while (hasNextToken() && brace_count > 0) {
//                     std::string t = getNextToken();
//                     if (t == "{") brace_count++;
//                     else if (t == "}") brace_count--;
//                 }
//             }
//         }
//     }
    
//     return servers;
// }


#include "config.hpp"
#include <cctype>
#include <cstdlib>

// Location class implementation
Location::Location() : _autoindex(false) {}

Location::~Location() {}

// Location getters
const std::string& Location::getPath() const { return _path; }
const std::vector<std::string>& Location::getMethods() const { return _methods; }
const std::string& Location::getRoot() const { return _root; }
bool Location::getAutoindex() const { return _autoindex; }
const std::vector<std::string>& Location::getIndexFiles() const { return _index_files; }
const std::string& Location::getUploadPath() const { return _upload_path; }
const std::map<std::string, std::string>& Location::getCgiExtensions() const { return _cgi_extensions; }
const std::string& Location::getRedirect() const { return _redirect; }

// Location setters
void Location::setPath(const std::string& path) { _path = path; }
void Location::setMethods(const std::vector<std::string>& methods) { _methods = methods; }
void Location::setRoot(const std::string& root) { _root = root; }
void Location::setAutoindex(bool autoindex) { _autoindex = autoindex; }
void Location::setIndexFiles(const std::vector<std::string>& index_files) { _index_files = index_files; }
void Location::setUploadPath(const std::string& upload_path) { _upload_path = upload_path; }
void Location::setCgiExtensions(const std::map<std::string, std::string>& cgi_extensions) { _cgi_extensions = cgi_extensions; }
void Location::setRedirect(const std::string& redirect) { _redirect = redirect; }
void Location::addCgiExtension(const std::string& extension, const std::string& path) { _cgi_extensions[extension] = path; }

void Location::print() const {
    std::cout << "    Location: " << _path << std::endl;
    std::cout << "      Methods: ";
    for (size_t i = 0; i < _methods.size(); ++i) {
        std::cout << _methods[i];
        if (i < _methods.size() - 1) std::cout << " ";
    }
    std::cout << std::endl;
    std::cout << "      Root: " << _root << std::endl;
    std::cout << "      Autoindex: " << (_autoindex ? "on" : "off") << std::endl;
    std::cout << "      Index files: ";
    for (size_t i = 0; i < _index_files.size(); ++i) {
        std::cout << _index_files[i];
        if (i < _index_files.size() - 1) std::cout << " ";
    }
    std::cout << std::endl;
    if (!_upload_path.empty())
        std::cout << "      Upload path: " << _upload_path << std::endl;
    if (!_redirect.empty())
        std::cout << "      Redirect: " << _redirect << std::endl;
}

// ServerConfig class implementation
ServerConfig::ServerConfig() : _host("127.0.0.1"), _port(80), _max_body_size(1024 * 1024) {}

ServerConfig::~ServerConfig() {}

// ServerConfig getters
const std::string& ServerConfig::getHost() const { return _host; }
int ServerConfig::getPort() const { return _port; }
const std::vector<std::string>& ServerConfig::getServerNames() const { return _server_names; }
const std::map<int, std::string>& ServerConfig::getErrorPages() const { return _error_pages; }
size_t ServerConfig::getMaxBodySize() const { return _max_body_size; }
const std::vector<Location>& ServerConfig::getLocations() const { return _locations; }

// ServerConfig setters
void ServerConfig::setHost(const std::string& host) { _host = host; }
void ServerConfig::setPort(int port) { _port = port; }
void ServerConfig::setServerNames(const std::vector<std::string>& server_names) { _server_names = server_names; }
void ServerConfig::setErrorPages(const std::map<int, std::string>& error_pages) { _error_pages = error_pages; }
void ServerConfig::setMaxBodySize(size_t max_body_size) { _max_body_size = max_body_size; }
void ServerConfig::setLocations(const std::vector<Location>& locations) { _locations = locations; }
void ServerConfig::addServerName(const std::string& server_name) { _server_names.push_back(server_name); }
void ServerConfig::addErrorPage(int code, const std::string& page) { _error_pages[code] = page; }
void ServerConfig::addLocation(const Location& location) { _locations.push_back(location); }

void ServerConfig::print() const {
    std::cout << "Server Configuration:" << std::endl;
    std::cout << "  Host: " << _host << std::endl;
    std::cout << "  Port: " << _port << std::endl;
    std::cout << "  Server names: ";
    for (size_t i = 0; i < _server_names.size(); ++i) {
        std::cout << _server_names[i];
        if (i < _server_names.size() - 1) std::cout << " ";
    }
    std::cout << std::endl;
    std::cout << "  Max body size: " << _max_body_size << std::endl;
    std::cout << "  Error pages:" << std::endl;
    for (std::map<int, std::string>::const_iterator it = _error_pages.begin(); 
         it != _error_pages.end(); ++it) {
        std::cout << "    " << it->first << ": " << it->second << std::endl;
    }
    std::cout << "  Locations:" << std::endl;
    for (size_t i = 0; i < _locations.size(); ++i) {
        _locations[i].print();
    }
    std::cout << std::endl;
}

void ConfigParser::tokenize(const std::string& content) {
    tokens.clear();
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
                tokens.push_back(current_token);
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
                tokens.push_back(current_token);
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
                tokens.push_back(current_token);
                current_token.clear();
            }
            tokens.push_back(std::string(1, c));
        } else {
            current_token += c;
        }
    }
    
    if (!current_token.empty()) {
        tokens.push_back(current_token);
    }
}

std::string ConfigParser::getCurrentToken() {
    if (current_token < tokens.size()) {
        return tokens[current_token];
    }
    return "";
}

std::string ConfigParser::getNextToken() {
    if (current_token < tokens.size()) {
        return tokens[current_token++];
    }
    return "";
}

bool ConfigParser::hasNextToken() {
    return current_token < tokens.size();
}

void ConfigParser::skipToken() {
    if (current_token < tokens.size()) {
        current_token++;
    }
}

std::vector<std::string> ConfigParser::parseStringList() {
    std::vector<std::string> result;
    
    while (hasNextToken() && getCurrentToken() != ";" && getCurrentToken() != "}") {
        result.push_back(getNextToken());
    }
    
    if (hasNextToken() && getCurrentToken() == ";") {
        skipToken(); // Skip semicolon
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
    if (hasNextToken() && getCurrentToken() == "{") {
        skipToken();
    }
    
    // Parse location directives
    while (hasNextToken() && getCurrentToken() != "}") {
        std::string directive = getNextToken();
        
        if (directive == "allow_methods") {
            location.setMethods(parseStringList());
        } else if (directive == "root") {
            if (hasNextToken()) {
                location.setRoot(getNextToken());
            }
            if (hasNextToken() && getCurrentToken() == ";") {
                skipToken();
            }
        } else if (directive == "autoindex") {
            if (hasNextToken()) {
                std::string value = getNextToken();
                location.setAutoindex(value == "on");
            }
            if (hasNextToken() && getCurrentToken() == ";") {
                skipToken();
            }
        } else if (directive == "index") {
            location.setIndexFiles(parseStringList());
        } else if (directive == "upload_path") {
            if (hasNextToken()) {
                location.setUploadPath(getNextToken());
            }
            if (hasNextToken() && getCurrentToken() == ";") {
                skipToken();
            }
        } else if (directive == "cgi_extension") {
            if (hasNextToken()) {
                std::string ext = getNextToken();
                if (hasNextToken()) {
                    std::string path = getNextToken();
                    location.addCgiExtension(ext, path);
                }
            }
            if (hasNextToken() && getCurrentToken() == ";") {
                skipToken();
            }
        } else if (directive == "return") {
            if (hasNextToken()) {
                location.setRedirect(getNextToken());
            }
            if (hasNextToken() && getCurrentToken() == ";") {
                skipToken();
            }
        } else {
            // Skip unknown directive
            while (hasNextToken() && getCurrentToken() != ";" && getCurrentToken() != "}") {
                skipToken();
            }
            if (hasNextToken() && getCurrentToken() == ";") {
                skipToken();
            }
        }
    }
    
    // Skip closing brace
    if (hasNextToken() && getCurrentToken() == "}") {
        skipToken();
    }
    
    return location;
}

ServerConfig ConfigParser::parseServerBlock() {
    ServerConfig config;
    
    // Expect opening brace
    if (hasNextToken() && getCurrentToken() == "{") {
        skipToken();
    }
    
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
            }
            if (hasNextToken() && getCurrentToken() == ";") {
                skipToken();
            }
        } else if (directive == "server_name") {
            config.setServerNames(parseStringList());
        } else if (directive == "error_page") {
            if (hasNextToken()) {
                int error_code = std::atoi(getNextToken().c_str());
                if (hasNextToken()) {
                    config.addErrorPage(error_code, getNextToken());
                }
            }
            if (hasNextToken() && getCurrentToken() == ";") {
                skipToken();
            }
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
            }
            if (hasNextToken() && getCurrentToken() == ";") {
                skipToken();
            }
        } else if (directive == "location") {
            config.addLocation(parseLocationBlock());
        } else {
            // Skip unknown directive
            while (hasNextToken() && getCurrentToken() != ";" && getCurrentToken() != "}") {
                skipToken();
            }
            if (hasNextToken() && getCurrentToken() == ";") {
                skipToken();
            }
        }
    }
    
    // Skip closing brace
    if (hasNextToken() && getCurrentToken() == "}") {
        skipToken();
    }
    
    return config;
}

std::vector<ServerConfig> ConfigParser::parse(const std::string& config_file) {
    std::vector<ServerConfig> servers;
    std::ifstream file(config_file.c_str());
    
    if (!file.is_open()) {
        std::cerr << "Error: Could not open config file: " << config_file << std::endl;
        return servers;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    file.close();
    
    tokenize(content);
    current_token = 0;
    
    while (hasNextToken()) {
        std::string token = getNextToken();
        if (token == "server") {
            servers.push_back(parseServerBlock());
        } else {
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