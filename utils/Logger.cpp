//
// Created by Lajuana Bespin on 10/30/21.
//

#include "Logger.h"
#include <iostream>

static std::string enumStrings[] = {
        "ALL",
        "INFO",
        "DEBUG",
        "WARNING",
        "ERROR"
};

void webservlogger(LogLevel logLvl, const char *log, ...)
{
    va_list args;
    va_start(args, log);
    ::printf("|%s| ", enumStrings[logLvl].c_str());
    ::vprintf(log, args);
    va_end(args);
}
