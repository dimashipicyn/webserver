#ifndef HTTP_H
#define HTTP_H

#include <string>
#include <map>
#include "TcpSocket.h"
#include "EventPool.h"


class Request;
class Response;

class HTTP
{
public:
    HTTP();
    virtual ~HTTP();

    void listen(const std::string& host);
    void handler(Request& request, Response &response);
    void start();

private:
    EventPool evPool_;
};

#endif // HTTP_H
