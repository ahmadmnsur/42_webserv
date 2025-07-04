// #ifndef CONFIG_HPP
// #define CONFIG_HPP

// #include <string>
// #include <vector>
// #include <map>
// #include <iostream>
// #include <fstream>
// #include <sstream>

// struct Location {
//     std::string path;
//     std::vector<std::string> methods;
//     std::string root;
//     bool autoindex;
//     std::vector<std::string> index_files;
//     std::string upload_path;
//     std::map<std::string, std::string> cgi_extensions;
//     std::string redirect;
    
//     Location() : autoindex(false) {}
    
//     void print() const {
//         std::cout << "    Location: " << path << std::endl;
//         std::cout << "      Methods: ";
//         for (size_t i = 0; i < methods.size(); ++i) {
//             std::cout << methods[i];
//             if (i < methods.size() - 1) std::cout << " ";
//         }
//         std::cout << std::endl;
//         std::cout << "      Root: " << root << std::endl;
//         std::cout << "      Autoindex: " << (autoindex ? "on" : "off") << std::endl;
//         std::cout << "      Index files: ";
//         for (size_t i = 0; i < index_files.size(); ++i) {
//             std::cout << index_files[i];
//             if (i < index_files.size() - 1) std::cout << " ";
//         }
//         std::cout << std::endl;
//         if (!upload_path.empty())
//             std::cout << "      Upload path: " << upload_path << std::endl;
//         if (!redirect.empty())
//             std::cout << "      Redirect: " << redirect << std::endl;
//     }
// };

// struct ServerConfig {
//     std::string host;
//     int port;
//     std::vector<std::string> server_names;
//     std::map<int, std::string> error_pages;
//     size_t max_body_size;
//     std::vector<Location> locations;
    
//     ServerConfig() : host("127.0.0.1"), port(80), max_body_size(1024 * 1024) {}
    
//     void print() const {
//         std::cout << "Server Configuration:" << std::endl;
//         std::cout << "  Host: " << host << std::endl;
//         std::cout << "  Port: " << port << std::endl;
//         std::cout << "  Server names: ";
//         for (size_t i = 0; i < server_names.size(); ++i) {
//             std::cout << server_names[i];
//             if (i < server_names.size() - 1) std::cout << " ";
//         }
//         std::cout << std::endl;
//         std::cout << "  Max body size: " << max_body_size << std::endl;
//         std::cout << "  Error pages:" << std::endl;
//         for (std::map<int, std::string>::const_iterator it = error_pages.begin(); 
//              it != error_pages.end(); ++it) {
//             std::cout << "    " << it->first << ": " << it->second << std::endl;
//         }
//         std::cout << "  Locations:" << std::endl;
//         for (size_t i = 0; i < locations.size(); ++i) {
//             locations[i].print();
//         }
//         std::cout << std::endl;
//     }
// };

// class ConfigParser {
// private:
//     std::vector<std::string> tokens;
//     size_t current_token;
    
//     void tokenize(const std::string& content);
//     std::string getCurrentToken();
//     std::string getNextToken();
//     bool hasNextToken();
//     void skipToken();
    
//     ServerConfig parseServerBlock();
//     Location parseLocationBlock();
//     std::vector<std::string> parseStringList();
    
// public:
//     std::vector<ServerConfig> parse(const std::string& config_file);
// };

// #endif

#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP

#include "ServerConfig.hpp"
#include "Location.hpp"
#include <string>
#include <vector>

class ConfigParser {
private:
    std::vector<std::string> _tokens;
    size_t _current_token;
    bool _has_errors;
    
    void tokenize(const std::string& content);
    std::string getCurrentToken();
    std::string getNextToken();
    bool hasNextToken();
    void skipToken();
    bool expectToken(const std::string& expected);
    void skipExtraSemicolons();
    bool checkForMultipleSemicolons();
    
    ServerConfig parseServerBlock();
    Location parseLocationBlock();
    std::vector<std::string> parseStringList();
    
public:
    ConfigParser();
    ~ConfigParser();
    std::vector<ServerConfig> parse(const std::string& config_file);
    bool hasErrors() const;
};

#endif