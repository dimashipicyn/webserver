//
// Created by Lajuana Bespin on 10/30/21.
//

#include "Logger.h"
#include <iostream>

namespace webserv{
    webserv::Logger logger;
}

webserv::Logger::Logger()
{
    std::string msg[] = {"ALL", "INFO", "WARNING","ERROR"};
    for (int i = 0; i < MAX_ELEM_LOG_LEVEL; ++i) {
        mLogLevelStr[i] = msg[i];
    }
}

void webserv::Logger::log(webserv::Logger::LogLevel lvl, const std::string &message)
{
    std::cout << mLogLevelStr[lvl] << ": " << message << std::endl;
}
