#ifndef LOCATION_HPP
#define LOCATION_HPP

#include <string>
#include <vector>
#include <map>

class Location {
private:
    std::string _path;
    std::vector<std::string> _methods;
    std::string _root;
    bool _autoindex;
    std::vector<std::string> _index_files;
    std::string _upload_path;
    std::map<std::string, std::string> _cgi_extensions;
    std::string _redirect;

public:
    Location();
    ~Location();
    
    // Getters
    const std::string& getPath() const;
    const std::vector<std::string>& getMethods() const;
    const std::string& getRoot() const;
    bool getAutoindex() const;
    const std::vector<std::string>& getIndexFiles() const;
    const std::string& getUploadPath() const;
    const std::map<std::string, std::string>& getCgiExtensions() const;
    const std::string& getRedirect() const;
    
    // Setters
    void setPath(const std::string& path);
    void setMethods(const std::vector<std::string>& methods);
    void setRoot(const std::string& root);
    void setAutoindex(bool autoindex);
    void setIndexFiles(const std::vector<std::string>& index_files);
    void setUploadPath(const std::string& upload_path);
    void setCgiExtensions(const std::map<std::string, std::string>& cgi_extensions);
    void setRedirect(const std::string& redirect);
    void addCgiExtension(const std::string& extension, const std::string& path);
    
    void print() const;
};

#endif