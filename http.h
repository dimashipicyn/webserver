#ifndef HTTP_H
#define HTTP_H

#include <string>
#include <map>
#include "TcpSocket.h"
#include "EventPool.h"


class Request;
class Response;

class Accepter : public IEventAcceptor
{
public:
    Accepter();
    virtual ~Accepter();
    virtual void accept(Session& session);

private:
    std::vector<TcpSocket>  listeners_;
    EventPool               evPool_;
};


class HTTP
{
public:
    HTTP();
    virtual ~HTTP();

    void listen(const std::string& host);
    void handler(Request& request, Response &response);
    void start();

private:
    Accepter                accepter;
};

#endif // HTTP_H
