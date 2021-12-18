#ifndef HTTP_H
#define HTTP_H

#include <string>
#include <map>
#include "TcpSocket.h"
#include "EventPool.h"


class Request;

///
/// callback interface class
///
/// conn - socket descriptor
/// req - http request
///
class IHandle {
public:
    virtual ~IHandle();
    virtual void handler(int conn, const Request& req) = 0;
};

class HTTP : public EventPool::IEventAcceptor
{
public:
    explicit HTTP(EventPool *evPool, const std::string& host);
    virtual ~HTTP();

    void handle(const std::string& path, IHandle *h);
    void start();

    virtual void    accept(EventPool *evPool, int sock, struct sockaddr *addr);
    IHandle*        getHandle(const std::string& path);

private:


    EventPool                           *evPool_;
    TcpSocket                           socket_;
    std::map<std::string, IHandle*>     handleMap_;
};

#endif // HTTP_H
