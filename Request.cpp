//
// Created by Lajuana Bespin on 11/15/21.
//

#include <fstream>
#include <iostream>
#include <vector>
#include <unistd.h>
#include "Request.h"
#include "utils.h"

Request::Request() : state(READING), MAX_BUFFER_SIZE(10240) {
}

Request::~Request() {

}

int Request::read(int fd) {
    const int buffersize = 1024;
    char buf[buffersize];
    int ret;

    if (state == READING) {
        ret = ::read(fd, buf, buffersize - 1);
        if ( ret == -1 ) {
            state = ERROR;
            return -1;
        }
        buf[ret] = '\0';
        buffer.str(buf);
        buffer >> m_Method >> m_Path >> m_Version;

        buffer.str("");
        buffer.clear();
        state = FINISH;
    }
    return 0;
}

Request::State Request::getState() const {
    return state;
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

void Request::reset() {
    //m_Method = "";
   // m_Path = "";
    //m_Version = "";
    state = READING;
}
