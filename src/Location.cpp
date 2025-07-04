#include "Location.hpp"
#include <iostream>

Location::Location() : _autoindex(false) {}

Location::~Location() {}

// Getters
const std::string& Location::getPath() const {
    return _path;
}

const std::vector<std::string>& Location::getMethods() const {
    return _methods;
}

const std::string& Location::getRoot() const {
    return _root;
}

bool Location::getAutoindex() const {
    return _autoindex;
}

const std::vector<std::string>& Location::getIndexFiles() const {
    return _index_files;
}

const std::string& Location::getUploadPath() const {
    return _upload_path;
}

const std::map<std::string, std::string>& Location::getCgiExtensions() const {
    return _cgi_extensions;
}

const std::string& Location::getRedirect() const {
    return _redirect;
}

// Setters
void Location::setPath(const std::string& path) {
    _path = path;
}

void Location::setMethods(const std::vector<std::string>& methods) {
    _methods = methods;
}

void Location::setRoot(const std::string& root) {
    _root = root;
}

void Location::setAutoindex(bool autoindex) {
    _autoindex = autoindex;
}

void Location::setIndexFiles(const std::vector<std::string>& index_files) {
    _index_files = index_files;
}

void Location::setUploadPath(const std::string& upload_path) {
    _upload_path = upload_path;
}

void Location::setCgiExtensions(const std::map<std::string, std::string>& cgi_extensions) {
    _cgi_extensions = cgi_extensions;
}

void Location::setRedirect(const std::string& redirect) {
    _redirect = redirect;
}

void Location::addCgiExtension(const std::string& extension, const std::string& path) {
    _cgi_extensions[extension] = path;
}

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