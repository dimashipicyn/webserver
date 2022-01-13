//
// Created by Lajuana Bespin on 11/15/21.
//

#ifndef WEBSERV_RESPONSE_H
#define WEBSERV_RESPONSE_H


class RequestConfig{};

#include <string>
#include <map>
#include <set>
#include "Request.h"
#include "ResponseHeader.hpp"
#include "settingsManager/SettingsManager.hpp"

extern SettingsManager *settingsManager;

class Response {
public:
    Response();
    Response(const Request&);
    Response& operator=(const Response& response) {
		_header = response._header;
		_output = response._output;
		return *this;
	};
    ~Response();
	const std::string& getContent() const;
//	void setHeader(std::string name, std::string value) {};

/*============Block moved to class HTTP==================================//
    void methodGET(const Request& request);
    void methodPOST(const Request& request);
    void methodDELETE(const Request& request);
    void methodNotAllowed(const Request& request);
    void BadRequest();
==========================================================================*/

	std::string 		getPath(const Request&) const;
	std::string 		getErrorPath(const Request&) const;
	std::string			getHeader();
	void 				setStatusCode(int);
	void 				setHeaderField(const std::string&, const std::string&);
	void 				setHeaderField(const std::string&, int);
	void				setContentType(const std::string& path);

private:
    Response(const Response& response) {(void)response;};

private:
    ResponseHeader	_header;
    std::string		_output;

	static std::map<std::string, std::string>	_contentType;
	static std::map<std::string, std::string>	initContentType();
};

#endif //WEBSERV_RESPONSE_H
