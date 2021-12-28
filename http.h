#ifndef HTTP_H
#define HTTP_H

#include <string>
#include <set>
#include "TcpSocket.h"
#include "EventPool.h"


class Request;
class Response;

class HTTP
{
public:
    HTTP(const std::string& host, std::int16_t port);
    virtual ~HTTP();

    void handler(Request& request, Response &response);
    void start();

private:
    EventPool   evPool_;

private:
	std::set<std::string> _methods;
	void methodGET(Request& request, Response& response);
	void methodPOST(Request& request, Response& response);
	void methodDELETE(Request& request, Response& response);
	void methodNotAllowed();
};

#endif // HTTP_H
