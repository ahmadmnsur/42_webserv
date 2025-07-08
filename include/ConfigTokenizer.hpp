#ifndef CONFIGTOKENIZER_HPP
#define CONFIGTOKENIZER_HPP

#include <string>
#include <vector>

class ConfigTokenizer {
private:
    std::vector<std::string> _tokens;
    size_t _current_token;
    
    void processCharacter(char c, const std::string& content, size_t& i, 
                         std::string& current_token, bool& in_string);
    void handleSpecialCharacters(char c, std::string& current_token);
    void skipComment(const std::string& content, size_t& i);

public:
    ConfigTokenizer();
    ~ConfigTokenizer();
    
    void tokenize(const std::string& content);
    void reset();
    
    std::string getCurrentToken();
    std::string getNextToken();
    bool hasNextToken();
    void skipToken();
    
    const std::vector<std::string>& getTokens() const;
    size_t getCurrentPosition() const;
    void setPosition(size_t position);
};

#endif