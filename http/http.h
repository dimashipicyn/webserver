#ifndef HTTP_H
#define HTTP_H

#include <string>
#include <map>

#include "EventPool.h"

class Request;
class Response;
class TcpSocket;
class Route;
struct Session;

class HTTP : public EventPool
{
public:
    HTTP();
    virtual ~HTTP();

    void listen(const std::string& host);
    void start();

protected:
    virtual void asyncAccept(TcpSocket& socket);
    virtual void asyncRead(int socket);
    virtual void asyncWrite(int socket);
    virtual void asyncEvent(int socket, uint16_t flags);

    Session*    getSessionByID(int id);
    void        closeSessionByID(int id);
    void        newSessionByID(int id, Session& session);

    void handler(Request& request, Response &response);
    void cgi(Request &request, Response& response, Route* route);
    void autoindex(Request &request, Response& response, Route* route);


    typedef std::map<int, Session> tdSessionMap;
private:
    tdSessionMap    sessionMap_;
};

#endif // HTTP_H
