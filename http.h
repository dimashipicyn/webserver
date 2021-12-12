#ifndef HTTP_H
#define HTTP_H

#include <string>
#include <map>
#include "TcpSocket.h"

namespace webserv {
    class EventPool;
}

class Accepter;
class Request;

class IHandle {
public:
    virtual void handler(int conn, const Request& req) = 0;
};

class HTTP
{
public:
    explicit HTTP(const std::string& host);
    virtual ~HTTP();

public:
    class Request;
    class Response;


    void handle(const std::string& path, IHandle *h);
    IHandle *getHandle(const std::string& path);
    void start();

private:

    webserv::EventPool                  *evpool_;
    webserv::TcpSocket                  socket_;
    std::map<std::string, IHandle*>     handleMap_;
    Accepter                            *accepter_;
};

#endif // HTTP_H
