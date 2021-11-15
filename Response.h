//
// Created by Lajuana Bespin on 11/15/21.
//

#ifndef WEBSERV_RESPONSE_H
#define WEBSERV_RESPONSE_H


#include <string>
#include <map>

class Response {
public:
    Response();
    ~Response();

    void write(int fd);

private:
    Response(const Response& response) {};
    Response& operator=(const Response& response) {};

private:
    std::string m;
};

#endif //WEBSERV_RESPONSE_H
