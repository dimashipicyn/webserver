//
// Created by Lajuana Bespin on 10/30/21.
//

#ifndef WEBSERV_LOGGER_H
#define WEBSERV_LOGGER_H
#include <string>
#include <vector>

namespace webserv {
    class Logger {
    public:
        enum LogLevel {
            ALL,
            INFO,
            WARNING,
            ERROR,
            MAX_ELEM_LOG_LEVEL
        };

    public:
        Logger();
        void log(enum LogLevel lvl, const std::string& message);

    private:
        std::string                 mLogLevelStr[MAX_ELEM_LOG_LEVEL];
    };

    extern Logger   logger;
}


#endif //WEBSERV_LOGGER_H
