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
	const std::string& getContent() const;
	void setHeader(std::string name, std::string value) {};
	void errorPage();

private:
    Response(const Response& response) {(void)response;};
    Response& operator=(const Response& response) {(void)response;return *this;};

private:
    std::string _output;

};

#endif //WEBSERV_RESPONSE_H
