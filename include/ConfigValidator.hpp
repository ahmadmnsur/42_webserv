#ifndef CONFIGVALIDATOR_HPP
#define CONFIGVALIDATOR_HPP

#include <string>
#include <vector>

class ConfigValidator {
private:
    bool _has_errors;
    
    void reportError(const std::string& message);
    bool isKnownDirective(const std::string& directive) const;

public:
    ConfigValidator();
    ~ConfigValidator();
    
    bool validateHttpMethod(const std::string& method);
    bool validateDirective(const std::string& directive, const std::string& context);
    bool validateToken(const std::string& expected, const std::string& actual);
    bool checkMultipleSemicolons(const std::string& current, const std::string& next);
    
    void resetErrors();
    bool hasErrors() const;
    void addError(const std::string& message);
};

#endif