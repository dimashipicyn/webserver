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

    void setContent(const std::string& s);
    const std::string& getContent();
    void reset();

private:
    Response(const Response& response) {(void)response;};
    Response& operator=(const Response& response) {(void)response;return *this;};

private:
    std::string content_;
};

#endif //WEBSERV_RESPONSE_H
