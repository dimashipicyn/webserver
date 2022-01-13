#ifndef HTTP_H
#define HTTP_H

#include <string>
#include <set>
#include "TcpSocket.h"
#include "EventPool.h"

#include "Response.h"
#include "ResponseHeader.hpp"
#include "settingsManager/SettingsManager.hpp"
#include "cgi/Cgi.hpp"



class Request;
class Response;

class HTTP
{
public:
    HTTP(const std::string& host, std::int16_t port);
    virtual ~HTTP();

    void handler(Request& request);
    void start();
	const Response& getResponse() const;

	//======http methods block moved from Response class============//
	void methodGET(const Request& request);
	void methodPOST(const Request& request);
	void methodDELETE(const Request& request);
	void methodNotAllowed(const Request& request);
	void BadRequest();
	//=============================================================//


private:
	Response    response_;
	EventPool   evPool_;

//===============Moved from class Response====================================
	static std::map<std::string, void (HTTP::*)(const Request &)>	_method;
	static std::map<std::string, void (HTTP::*)(const Request &)>	initMethods();

	static std::set<std::string> _allMethods;
	static std::set<std::string> initAllMethods();
//===============================================================================
};

#endif // HTTP_H
