#ifndef RESPONSEHEADER_HPP
#define RESPONSEHEADER_HPP

#include <map>
#include <set>
#include <string>
#include "Request.h"

class ResponseHeader {
public:
	ResponseHeader(void);
	ResponseHeader(const ResponseHeader & src);
	~ResponseHeader(void);

	ResponseHeader & operator=(const ResponseHeader & src);

	// Setter functions
	void	        setHeader(const std::string&, const std::string&);
    void            setHeader(const std::string&, const int&);
    void            setCode(int);
//	void			setContentLocation(const std::string& path, int code);
	void			setContentType(const Request&, std::string path);
	std::string		getDate(void);
//	void			setLastModified(const std::string& path);
//	void			setLocation(int code, const std::string& redirect);
//	void			setRetryAfter(int code, int sec);
//	void			setWwwAuthenticate(int code);


	// Member functions
	std::string		getHeader(const Request& request);
	std::string		writeHeader(void);


	static std::map<int, std::string>	_errors;
	static std::map<int, std::string>	initErrorMap();


private:
	int											_code;
	static std::map<std::string, std::string>	_headers;
	static std::map<std::string, std::string>	resetHeaders();

	static std::map<std::string, std::string>	_contentType;
	static std::map<std::string, std::string>	initContentType();
};


#endif //WEBSERV_RESPONSEHEADER_HPP
