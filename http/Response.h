//
// Created by Lajuana Bespin on 11/15/21.
//

#ifndef WEBSERV_RESPONSE_H
#define WEBSERV_RESPONSE_H


#include <string>
#include <map>

class Request;
class Response {
public:
    Response();
    ~Response();

    void	setContent(const std::string& s);
    const	std::string&	getContent();
	void	buildErrorPage(int code, const Request&);
	void	buildDelPage(const Request&);

	std::string 		getErrorPath(const Request&) const;
	std::string			getHeader();
	void 				setStatusCode(int);
	void 				setHeaderField(const std::string&, const std::string&);
	void 				setHeaderField(const std::string&, int);
	void				setContentType(const std::string& path);
    void                writeFile(const std::string& path, const std::string body);
	void 				writeContent(const std::string& content, const Request&);
	void 				setBody(const std::string&);
	const std::string&	getBody();


private:
    Response(const Response& response) {(void)response;};
    Response& operator=(const Response& response) {(void)response;return *this;};

private:
    std::string content_;
	std::string body_;
	std::string header_;
	int			statusCode_;

	static std::map<std::string, std::string>	_contentType;
	static std::map<std::string, std::string>	initContentType();

	static std::map<int, std::string>	reasonPhrase;
	static std::map<int, std::string>	initErrorMap();
};

#endif //WEBSERV_RESPONSE_H
