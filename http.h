#ifndef HTTP_H
#define HTTP_H

#include <string>
#include <map>
#include "TcpSocket.h"
#include "EventPool.h"


class Request;
class Response;
struct Session;

class HTTP : public EventPool
{
public:
    HTTP();
    virtual ~HTTP();

    void listen(const std::string& host);
    void handler(Request& request, Response &response);
    void start();

protected:
    virtual void asyncAccept(int socket);
    virtual void asyncRead(int socket);
    virtual void asyncWrite(int socket);
    virtual void asyncEvent(int socket, uint16_t flags);

    Session*    getSessionByID(int id);
    void        newSession(std::auto_ptr<TcpSocket>& socket);

    typedef std::map<int, Session> tdSessionMap;
private:
    tdSessionMap    sessionMap_;
};

#endif // HTTP_H
