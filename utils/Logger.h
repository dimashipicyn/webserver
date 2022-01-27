//
// Created by Lajuana Bespin on 10/30/21.
//

#ifndef WEBSERV_LOGGER_H
#define WEBSERV_LOGGER_H
#include <string>
#include <vector>

enum LogLevel {
    ALL_LOG_LVL,
    INFO_LOG_LVL,
    DEBUG_LOG_LVL,
    WARNING_LOG_LVL,
    ERROR_LOG_LVL,
    MAX_ELEM_LOG_LEVEL
};

#define LOG(loglevel, log, args...) webservlogger(loglevel, log, args);
//#define LOG_DEBUG(args...) webservlogger(DEBUG_LOG_LVL, args);
#define LOG_DEBUG(args...)
#define LOG_INFO(args...) webservlogger(INFO_LOG_LVL, args);
#define LOG_WARNING(args...) webservlogger(WARNING_LOG_LVL, args);
#define LOG_ERROR(args...) webservlogger(ERROR_LOG_LVL, args);

void webservlogger(LogLevel logLvl, const char *log, ...);

#endif //WEBSERV_LOGGER_H
