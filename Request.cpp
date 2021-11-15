//
// Created by Lajuana Bespin on 11/15/21.
//

#include <fstream>
#include <iostream>
#include <vector>
#include "Request.h"
#include "utils.h"

Request::Request() {

}

Request::~Request() {

}

void Request::read(int fd) {
    std::string line;
    get_next_line(fd, line);
    std::vector<std::string> words = split(line, ' ');
    m_Method = words[0];
    m_Path = words[1];
    m_Version = words[2];
}

std::ostream& operator<<(std::ostream& os, const Request& request) {
    os << request.getMethod() << " "
    << request.getPath() << " "
    << request.getVersion() << std::endl;
    return os;
}

const std::string &Request::getMethod() const {
    return m_Method;
}

const std::string &Request::getVersion() const {
    return m_Version;
}

const std::string &Request::getPath() const {
    return m_Path;
}
