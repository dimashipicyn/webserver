#ifndef HTTP_H
#define HTTP_H

#include <string>
#include <map>
#include <set>

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


	//===========================Moved from web.1.0 ==========================//
	void methodGET(const Request&, Response&, Route&);
	void methodPOST(const Request&, Response&, Route&);
	void methodDELETE(const Request&, Response&, Route&);
	void methodNotAllowed(const Request&, Response&);
	void BadRequest(Response&);

	static std::map<std::string, void (HTTP::*)(const Request &, Response&, Route&)>	_method;
	static std::map<std::string, void (HTTP::*)(const Request &, Response&, Route&)>	initMethods();

	static std::set<std::string> _allMethods;
	static std::set<std::string> initAllMethods();
	//==========================================================================


};

#endif // HTTP_H
