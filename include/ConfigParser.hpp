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
    bool isValidHttpMethod(const std::string& method) const;
    
    ServerConfig parseServerBlock();
    Location parseLocationBlock();
    std::vector<std::string> parseStringList();
    std::vector<std::string> parseHttpMethods();
    
public:
    ConfigParser();
    ~ConfigParser();
    std::vector<ServerConfig> parse(const std::string& config_file);
    bool hasErrors() const;
};

#endif