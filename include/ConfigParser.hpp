#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP

#include "ServerConfig.hpp"
#include "Location.hpp"
#include "ConfigTokenizer.hpp"
#include "ConfigValidator.hpp"
#include <string>
#include <vector>

class ConfigParser {
private:
    ConfigTokenizer _tokenizer;
    ConfigValidator _validator;
    
    bool expectToken(const std::string& expected);
    void skipExtraSemicolons();
    bool checkForMultipleSemicolons();
    void skipToEndOfBlock();
    
    ServerConfig parseServerBlock();
    Location parseLocationBlock();
    std::vector<std::string> parseStringList();
    std::vector<std::string> parseHttpMethods();
    
    std::string getCurrentToken();
    std::string getNextToken();
    bool hasNextToken();
    void skipToken();
    
public:
    ConfigParser();
    ~ConfigParser();
    std::vector<ServerConfig> parse(const std::string& config_file);
    bool hasErrors() const;
};

#endif