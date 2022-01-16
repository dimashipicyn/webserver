#include "ResponseHeader.hpp"
#include "SettingsManager.hpp"
#include <sstream>
#include <sys/time.h>
#include "Request.h"

std::map<int, std::string>	ResponseHeader::initErrorMap(){
	std::map<int, std::string> errors;
	errors[100] = "Continue";
	errors[200] = "OK";
	errors[201] = "Created";
	errors[204] = "No Content";
	errors[400] = "Bad Request";
	errors[403] = "Forbidden";
	errors[404] = "Not Found";
	errors[405] = "Method Not Allowed";
	errors[413] = "Payload Too Large";
	errors[500] = "Internal Server Error";
	return errors;
}

std::map<int, std::string> ResponseHeader::_errors
					= ResponseHeader::initErrorMap();

	std::string		ResponseHeader::getHeader(){
	std::ostringstream header;

	header << "HTTP/1.1 " << _code << " " << _errors[_code] << "\r\n";
	header << buildHeader();
	return header.str();
}

void			ResponseHeader::setStatusCode(int code){ _code = code; }

std::string		ResponseHeader::buildHeader(void)
{
	std::ostringstream	header;
	for (std::map<std::string, std::string>::const_iterator it = _header.cbegin();
												it != _header.cend(); ++it) {
			header << it->first << ": " << it->second << "\n";
	}
	header << "\r\n";
	return header.str();
}

void ResponseHeader::setHeaderField(const std::string &key, const std::string &value) {
	_header[key] = value;
}

void ResponseHeader::setHeaderField(const std::string &key, const int &value) {
    std::ostringstream oss;
	oss << value;
    _header[key] = oss.str();
}

std::string			ResponseHeader::getDate(void){
	char			buffer[100];
	struct timeval	tv;
	struct tm		*tm;

	gettimeofday(&tv, NULL);
	tm = gmtime(&tv.tv_sec);
	strftime(buffer, 100, "%a, %d %b %Y %H:%M:%S GMT", tm);
	return std::string(buffer);
}
/*
void			ResponseHeader::setLastModified(const std::string& path){
	char			buffer[100];
	struct stat		stats;
	struct tm		*tm;

	if (stat(path.c_str(), &stats) == 0){
		tm = gmtime(&stats.st_mtime);
		strftime(buffer, 100, "%a, %d %b %Y %H:%M:%S GMT", tm);
		_headers["Last-Modified"] = std::string(buffer);
	}
}*/
/*
void	ResponseHeader::setLocation(int code, const std::string& redirect){
	if (code == 201 || code / 100 == 3){
		_headers["Location"] = redirect;
	}
}*/
/*
void	ResponseHeader::setRetryAfter(int code, int sec){
	if (code == 503 || code == 429 || code == 301){
        setHeader("Retry-After", sec);
	}
}*/
/*
void	ResponseHeader::setWwwAuthenticate(int code){
	if (code == 401){
		_headers["WWW-Authenticate"] = "Basic realm=\"Access requires authentification\" charset=\"UTF-8\"";
	}
}
*/

ResponseHeader & ResponseHeader::operator=(const ResponseHeader & src){
	(void)src;
	return (*this);
}

		// Constructors and destructors

ResponseHeader::ResponseHeader(void){}
ResponseHeader::ResponseHeader(const ResponseHeader & src){ (void)src; }
ResponseHeader::~ResponseHeader(void){}
