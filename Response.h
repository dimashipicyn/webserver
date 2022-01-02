//
// Created by Lajuana Bespin on 11/15/21.
//

#ifndef WEBSERV_RESPONSE_H
#define WEBSERV_RESPONSE_H


class RequestConfig{};

#include <string>
#include <map>
#include <set>
#include "Request.h"
#include "ResponseHeader.hpp"

class Response {
public:
    Response();
    Response(const Request&, const RequestConfig&);
    Response& operator=(const Response& response) {(void)response;return *this;};
    ~Response();
	const std::string& getContent() const;
	void setHeader(std::string name, std::string value) {};
	void methodGET(const Request& request);

    void methodGET(const Request& request, const RequestConfig &);
    void methodPOST(const Request& request, const RequestConfig &);
    void methodDELETE(const Request& request, const RequestConfig &);
    void methodNotAllowed(const Request& request, const RequestConfig &);
    void BadRequest();

private:
    Response(const Response& response) {(void)response;};


private:
    ResponseHeader _header;
    std::string _output;

    static std::map<std::string, void (Response::*)(const Request &, const RequestConfig &)>	_method;
    static std::map<std::string, void (Response::*)(const Request &, const RequestConfig &)>	initMethods();

    static std::set<std::string> _allMethods;
    static std::set<std::string> initAllMethods();

};

#endif //WEBSERV_RESPONSE_H
