//
// Created by Lajuana Bespin on 11/15/21.
//

#ifndef WEBSERV_RESPONSE_H
#define WEBSERV_RESPONSE_H


#include <string>
#include <map>
#include "ResponseHeader.hpp"

class Request;
class Response {
public:
    Response();
    ~Response();

    void setContent(const std::string& s);
    const std::string&	getContent();
	std::string	getErrorPage(int);
    void reset();

	std::string 		getErrorPath(const Request&) const;
	std::string			getHeader();
	void 				setStatusCode(int);
	void 				setHeaderField(const std::string&, const std::string&);
	void 				setHeaderField(const std::string&, int);
	void				setContentType(const std::string& path);


private:
    Response(const Response& response) {(void)response;};
    Response& operator=(const Response& response) {(void)response;return *this;};

private:
    std::string content_;
	ResponseHeader header_;



	static std::map<std::string, std::string>	_contentType;
	static std::map<std::string, std::string>	initContentType();
};

#endif //WEBSERV_RESPONSE_H
