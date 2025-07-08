#include "Location.hpp"
#include <iostream>

/*
 * Default constructor for Location
 * Initializes with autoindex disabled by default
 */
Location::Location() : _autoindex(false) {}

/*
 * Destructor for Location
 * Cleans up any allocated resources
 */
Location::~Location() {}

// Getters
/*
 * Returns the URL path pattern for this location block
 * Used for request routing and matching
 */
const std::string& Location::getPath() const {
    return _path;
}

/*
 * Returns the list of allowed HTTP methods for this location
 * Used for validating incoming requests
 */
const std::vector<std::string>& Location::getMethods() const {
    return _methods;
}

/*
 * Returns the root directory for serving files in this location
 * Used for file system path resolution
 */
const std::string& Location::getRoot() const {
    return _root;
}

/*
 * Returns whether directory listing is enabled for this location
 * Used for serving directory contents when no index file is found
 */
bool Location::getAutoindex() const {
    return _autoindex;
}

/*
 * Returns the list of index files to try when serving directories
 * Used for serving default files in directories
 */
const std::vector<std::string>& Location::getIndexFiles() const {
    return _index_files;
}

/*
 * Returns the path where uploaded files should be stored
 * Used for handling file uploads via POST requests
 */
const std::string& Location::getUploadPath() const {
    return _upload_path;
}

/*
 * Returns the map of CGI file extensions to their interpreter paths
 * Used for executing CGI scripts based on file extension
 */
const std::map<std::string, std::string>& Location::getCgiExtensions() const {
    return _cgi_extensions;
}

/*
 * Returns the redirect URL or status code for this location
 * Used for HTTP redirections
 */
const std::string& Location::getRedirect() const {
    return _redirect;
}

// Setters
/*
 * Sets the URL path pattern for this location block
 * Called during configuration parsing
 */
void Location::setPath(const std::string& path) {
    _path = path;
}

/*
 * Sets the list of allowed HTTP methods for this location
 * Called during configuration parsing
 */
void Location::setMethods(const std::vector<std::string>& methods) {
    _methods = methods;
}

/*
 * Sets the root directory for serving files in this location
 * Called during configuration parsing
 */
void Location::setRoot(const std::string& root) {
    _root = root;
}

/*
 * Sets whether directory listing is enabled for this location
 * Called during configuration parsing
 */
void Location::setAutoindex(bool autoindex) {
    _autoindex = autoindex;
}

/*
 * Sets the list of index files to try when serving directories
 * Called during configuration parsing
 */
void Location::setIndexFiles(const std::vector<std::string>& index_files) {
    _index_files = index_files;
}

/*
 * Sets the path where uploaded files should be stored
 * Called during configuration parsing
 */
void Location::setUploadPath(const std::string& upload_path) {
    _upload_path = upload_path;
}

/*
 * Sets the map of CGI file extensions to their interpreter paths
 * Called during configuration parsing
 */
void Location::setCgiExtensions(const std::map<std::string, std::string>& cgi_extensions) {
    _cgi_extensions = cgi_extensions;
}

/*
 * Sets the redirect URL or status code for this location
 * Called during configuration parsing
 */
void Location::setRedirect(const std::string& redirect) {
    _redirect = redirect;
}

/*
 * Adds a CGI extension mapping to the location
 * Called when parsing multiple CGI extension directives
 */
void Location::addCgiExtension(const std::string& extension, const std::string& path) {
    _cgi_extensions[extension] = path;
}

/*
 * Prints the location configuration to stdout for debugging
 * Displays all configured values including methods, root, and CGI extensions
 */
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
    if (!_cgi_extensions.empty()) {
        std::cout << "      CGI extensions:" << std::endl;
        for (std::map<std::string, std::string>::const_iterator it = _cgi_extensions.begin(); 
             it != _cgi_extensions.end(); ++it) {
            std::cout << "        " << it->first << ": " << it->second << std::endl;
        }
    }
    if (!_redirect.empty())
        std::cout << "      Redirect: " << _redirect << std::endl;
}