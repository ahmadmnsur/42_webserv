#include "ConfigTokenizer.hpp"
#include <cctype>

/*
 * Default constructor for ConfigTokenizer
 * Initializes the tokenizer with position 0
 */
ConfigTokenizer::ConfigTokenizer() : _current_token(0) {}

/*
 * Destructor for ConfigTokenizer
 * Cleans up allocated resources
 */
ConfigTokenizer::~ConfigTokenizer() {}

/*
 * Processes a single character during tokenization
 * Handles quoted strings, whitespace, comments, and special characters
 * Updates the current token being built and manages string state
 */
void ConfigTokenizer::processCharacter(char c, const std::string& content, size_t& i, 
                                      std::string& current_token, bool& in_string) {
    if (c == '"' && !in_string) {
        in_string = true;
    } else if (c == '"' && in_string) {
        in_string = false;
        if (!current_token.empty()) {
            _tokens.push_back(current_token);
            current_token.clear();
        }
    } else if (in_string) {
        current_token += c;
    } else if (std::isspace(c)) {
        if (!current_token.empty()) {
            _tokens.push_back(current_token);
            current_token.clear();
        }
    } else if (c == '#') {
        if (!current_token.empty()) {
            _tokens.push_back(current_token);
            current_token.clear();
        }
        skipComment(content, i);
    } else {
        handleSpecialCharacters(c, current_token);
    }
}

/*
 * Handles special characters like braces and semicolons
 * Treats them as separate tokens while preserving the current token
 * Adds regular characters to the current token being built
 */
void ConfigTokenizer::handleSpecialCharacters(char c, std::string& current_token) {
    if (c == '{' || c == '}' || c == ';') {
        if (!current_token.empty()) {
            _tokens.push_back(current_token);
            current_token.clear();
        }
        _tokens.push_back(std::string(1, c));
    } else {
        current_token += c;
    }
}

/*
 * Skips comments in the configuration file
 * Advances the position until end of line or end of content
 */
void ConfigTokenizer::skipComment(const std::string& content, size_t& i) {
    while (i < content.length() && content[i] != '\n') {
        ++i;
    }
}

/*
 * Main tokenization function that processes the entire content
 * Breaks down the configuration file into individual tokens
 * Handles strings, comments, and special characters
 */
void ConfigTokenizer::tokenize(const std::string& content) {
    _tokens.clear();
    _current_token = 0;
    
    std::string current_token;
    bool in_string = false;
    
    for (size_t i = 0; i < content.length(); ++i) {
        processCharacter(content[i], content, i, current_token, in_string);
    }
    
    if (!current_token.empty()) {
        _tokens.push_back(current_token);
    }
}

/*
 * Resets the tokenizer position to the beginning
 * Allows re-parsing of the same token stream
 */
void ConfigTokenizer::reset() {
    _current_token = 0;
}

/*
 * Returns the current token without advancing position
 * Returns empty string if no more tokens available
 */
std::string ConfigTokenizer::getCurrentToken() {
    if (_current_token < _tokens.size()) {
        return _tokens[_current_token];
    }
    return "";
}

/*
 * Returns the current token and advances to the next one
 * Returns empty string if no more tokens available
 */
std::string ConfigTokenizer::getNextToken() {
    if (_current_token < _tokens.size()) {
        return _tokens[_current_token++];
    }
    return "";
}

/*
 * Checks if there are more tokens available for consumption
 * Returns true if more tokens exist, false otherwise
 */
bool ConfigTokenizer::hasNextToken() {
    return _current_token < _tokens.size();
}

/*
 * Skips the current token and advances to the next one
 * Does not return the token value, just advances position
 */
void ConfigTokenizer::skipToken() {
    if (_current_token < _tokens.size()) {
        _current_token++;
    }
}

/*
 * Returns a reference to the complete token vector
 * Used for debugging or advanced token manipulation
 */
const std::vector<std::string>& ConfigTokenizer::getTokens() const {
    return _tokens;
}

/*
 * Returns the current position in the token stream
 * Used for position tracking and debugging
 */
size_t ConfigTokenizer::getCurrentPosition() const {
    return _current_token;
}

/*
 * Sets the current position in the token stream
 * Allows seeking to a specific position for backtracking
 */
void ConfigTokenizer::setPosition(size_t position) {
    _current_token = position;
}