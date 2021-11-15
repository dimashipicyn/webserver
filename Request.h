//
// Created by Lajuana Bespin on 11/15/21.
//

#ifndef WEBSERV_REQUEST_H
#define WEBSERV_REQUEST_H

#include <map>
#include <string>

class Request {
public:
    Request();
    ~Request();

    void read(int fd);

    const std::string &getMethod() const;

    const std::string &getVersion() const;

    const std::string &getPath() const;

private:
    Request(const Request& request) {};
    Request& operator=(const Request& request) {};

private:
    std::string                         m_Method;
    std::string                         m_Version;
    std::string                         m_Path;
    std::map<std::string, std::string>  m_Headers;
};

std::ostream& operator<<(std::ostream& os, const Request& request);

#endif //WEBSERV_REQUEST_H
