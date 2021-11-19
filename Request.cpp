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

    switch (state) {
        case READING:
            ret = ::read(fd, buf, buffersize);
            if ( ret == -1 ) {
                return -1;
            }
            buf[ret] = '\0';
            buffer.append(buf);

            if ( buffer.size() >= MAX_BUFFER_SIZE ||
                 buffer.find("\n\n") != std::string::npos ||
                 buffer.find("\r\n\r\n") != std::string::npos )
            {
                state = PARSING;
            }
            std::cout << "Reading" << std::endl;
            break;
        case PARSING:
            std::cout << "Parsing" << std::endl;
            state = FINISH;
            break;
        case FINISH:
            std::cout << "Finish" << std::endl;
            break;
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
