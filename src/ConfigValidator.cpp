#include "ConfigValidator.hpp"
#include <iostream>

/*
 * Default constructor for ConfigValidator
 * Initializes the validator with no errors
 */
ConfigValidator::ConfigValidator() : _has_errors(false) {}

/*
 * Destructor for ConfigValidator
 * Cleans up any allocated resources
 */
ConfigValidator::~ConfigValidator() {}

/*
 * Reports an error message to stderr and sets error flag
 * Used internally by validation functions
 */
void ConfigValidator::reportError(const std::string& message) {
    std::cerr << "Error: " << message << std::endl;
    _has_errors = true;
}

/*
 * Checks if a directive is known and valid
 * Returns true for recognized configuration directives
 */
bool ConfigValidator::isKnownDirective(const std::string& directive) const {
    return (directive == "root" || directive == "autoindex" || directive == "index" || 
            directive == "methods" || directive == "allow_methods" || directive == "upload_path" || 
            directive == "cgi_extension" || directive == "cgi_extensions" || directive == "return" || 
            directive == "listen" || directive == "server_name" || directive == "error_page" || 
            directive == "client_max_body_size" || directive == "location");
}

/*
 * Validates HTTP methods according to project requirements
 * Only allows GET, POST, and DELETE methods
 * Returns true if method is valid, false otherwise
 */
bool ConfigValidator::validateHttpMethod(const std::string& method) {
    if (method != "GET" && method != "POST" && method != "DELETE") {
        reportError("Invalid HTTP method '" + method + "'. Valid methods are: GET, POST, DELETE");
        return false;
    }
    return true;
}

/*
 * Validates a directive within a specific context
 * Checks for unexpected semicolons and unknown directives
 * Returns true if directive is valid, false otherwise
 */
bool ConfigValidator::validateDirective(const std::string& directive, const std::string& context) {
    if (directive == ";") {
        reportError("Unexpected semicolon. Multiple consecutive semicolons are not allowed.");
        return false;
    }
    
    if (!isKnownDirective(directive)) {
        reportError("Unknown directive '" + directive + "' in " + context + " block");
        return false;
    }
    
    return true;
}

/*
 * Validates that a token matches the expected value
 * Reports appropriate error messages for mismatches
 * Returns true if token matches, false otherwise
 */
bool ConfigValidator::validateToken(const std::string& expected, const std::string& actual) {
    if (expected != actual) {
        if (actual.empty()) {
            reportError("Expected '" + expected + "' but reached end of file");
        } else {
            reportError("Expected '" + expected + "' but found '" + actual + "'");
        }
        return false;
    }
    return true;
}

/*
 * Checks for multiple consecutive semicolons
 * Reports error if consecutive semicolons are found
 * Returns true if no consecutive semicolons, false otherwise
 */
bool ConfigValidator::checkMultipleSemicolons(const std::string& current, const std::string& next) {
    if (current == ";" && next == ";") {
        reportError("Multiple consecutive semicolons found. Use only one semicolon after each directive.");
        return false;
    }
    return true;
}

/*
 * Resets the error state to allow reuse of the validator
 * Clears any previously reported errors
 */
void ConfigValidator::resetErrors() {
    _has_errors = false;
}

/*
 * Checks if any errors have been reported during validation
 * Returns true if errors exist, false otherwise
 */
bool ConfigValidator::hasErrors() const {
    return _has_errors;
}

/*
 * Adds an error to the validator's error state
 * Convenience function that wraps reportError
 */
void ConfigValidator::addError(const std::string& message) {
    reportError(message);
}