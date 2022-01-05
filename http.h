#ifndef HTTP_H
#define HTTP_H

#include <string>
#include <set>
#include "TcpSocket.h"
#include "EventPool.h"
#include "Response.h"


class Request;
class Response;

class HTTP
{
public:
    HTTP(const std::string& host, std::int16_t port);
    virtual ~HTTP();

    void handler(Request& request);
    void start();

private:
    EventPool   evPool_;
    Response    _response;

/*
	std::set<std::string> _methods;

/*
    void methodGET(Request& request, Response& response);
	void methodPOST(Request& request, Response& response);
	void methodDELETE(Request& request, Response& response);
	void methodNotAllowed(Request& request, Response& response);
	void methodBadRequest(); */
};

#endif // HTTP_H
