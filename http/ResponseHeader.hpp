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

	void	        setHeaderField(const std::string&, const std::string&);
    void            setHeaderField(const std::string&, const int&);
    void            setStatusCode(int);
	std::string		getDate(void);


	std::string		getHeader();
	std::string				buildHeader(void);

	static std::map<int, std::string>	_errors;
	static std::map<int, std::string>	initErrorMap();

private:
	int											_code;
	std::map<std::string, std::string>			_header;

};


#endif //WEBSERV_RESPONSEHEADER_HPP
