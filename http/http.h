#ifndef HTTP_H
#define HTTP_H

#include <string>
#include <map>
#include <set>
#include <vector>

#include "EventPool.h"

class Request;
class Response;
class TcpSocket;
class Route;
struct Session;

class HTTP : public EventPool
{
	friend struct Session;

public:
	HTTP();
	virtual ~HTTP();

	static void startServer();
	static bool isValidMethod(const std::string &method);

protected:
	virtual void asyncAccept(TcpSocket& socket);
	virtual void asyncRead(int socket);
	virtual void asyncWrite(int socket);
	virtual void asyncEvent(int socket, uint16_t flags);

    void defaultReadCallback(int socket, Session* session);
    void defaultWriteCallback(int socket, Session* session);
    void doneWriteCallback(int socket, Session* session);

    void readBodyEventRead(int socket, Session* session);
    void readBodyEventWrite(int socket, Session* session);

    void readBodyChunkedEventRead(int socket, Session* session);
    void readBodyChunkedEventWrite(int socket, Session* session);


	Session*    getSessionByID(int id);
	void        closeSessionByID(int id);
    void        newSessionByID(int id, Session& session);



	void listen(const std::string& host);
	void start();

	void	handler(Request& request, Response &response);
	bool	cgi(const Request &request, Response& response, Route* route);
	bool	autoindex(const Request &request, Response& response, Route* route);
    int		redirection(const std::string &from, std::string &to, Route* route);
	void 	interpretResponseString(std::string const &responseString, Response &response);


    void requestValidate(Request& request);


	typedef std::map<int, Session> tdSessionMap;
private:
	tdSessionMap    sessionMap_;


	//===========================Moved from web.1.0 ==========================//
	void methodGET(const Request&, Response&, Route*);
	void methodPOST(const Request&, Response&, Route*);
	void methodDELETE(const Request&, Response&, Route*);
	void methodPUT(const Request&, Response&, Route*);
	void methodHEAD(const Request&, Response&, Route*);
	void methodCONNECT(const Request&, Response&, Route*);
	void methodOPTIONS(const Request&, Response&, Route*);
	void methodTRACE(const Request&, Response&, Route*);
	void methodPATCH(const Request&, Response&, Route*);
    void checkIfAllowed(const std::string&, Route*);
//	void methodNotAllowed(const Request&, Response&);
//	void BadRequest(Response&);

    void methodsCaller(Request& request, Response& response, Route* route);


	typedef std::map<std::string, void (HTTP::*)(const Request &, Response&, Route*)> MethodHttp;
	static MethodHttp _method;
	static MethodHttp 	initMethods();
	//==========================================================================


};

#endif // HTTP_H
