//
// Created by Lajuana Bespin on 10/30/21.
//

#ifndef WEBSERV_JOB_H
#define WEBSERV_JOB_H
#include <string>


namespace webserv {
    struct Job {
        std::string     response;
        std::string     request;
    };
}

#endif //WEBSERV_JOB_H
