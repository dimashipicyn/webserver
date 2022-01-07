#include "ResponseHeader.hpp"
#include <sstream>
#include <sys/time.h>
#include "Request.h"

extern SettingsManager *settingsManager;

std::map<std::string, std::string>	ResponseHeader::resetHeaders(void){
	std::map<std::string, std::string> header;
	header["Allow"] = "";
	header["Content-Language"]  = "";
	header["Content-Length"] = "";
	header["Content-Location"] = "";
	header["Content-Type"] = "";
	header["Date"] = "";
	header["Last-Modified"] = "";
	header["Location"] = "";
	header["Retry-After"] = "";
	header["Server"] = "";
	header["Transfer-Encoding"] = "";
	header["WWW-Authenticate"] = "";
	return header;
}

std::map<std::string, std::string> ResponseHeader::_headers
									= ResponseHeader::resetHeaders();

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

std::map<std::string, std::string> ResponseHeader::initContentType() {
    std::map<std::string, std::string> contentType;

    contentType["html"] = "text/html";
    contentType["css"] = "text/css";
    contentType["js"] = "text/javascript";
    contentType["jpeg"] = "image/jpeg";
    contentType["jpg"] = "image/jpeg";
    contentType["png"] = "image/png";
    contentType["bmp"] = "image/bmp";
    contentType["text/plain"] = "text/plain";
    return contentType;
}

std::map<std::string, std::string> ResponseHeader::_contentType
						= ResponseHeader::initContentType();

std::string		ResponseHeader::getHeader(const Request& request){
	std::ostringstream header;

	setValues(request);
	header << "HTTP/1.1 " << _code << " " << _errors[_code] << "\r\n";
	header << writeHeader();
	return (header.str());
}

void	ResponseHeader::setCode(int code){ _code = code; }

std::string		ResponseHeader::writeHeader(void)
{
	std::ostringstream	header;

	for (std::map<std::string, std::string>::const_iterator it = _headers.cbegin();
												it != _headers.cend(); ++it) {
		if ( !it->second.empty() )
			header << it->first << ": " << it->second << "\r\n";
	}
	return header.str();
}

void ResponseHeader::setHeader(const std::string &key, const std::string &value) {
	_headers[key] = value;
}

void ResponseHeader::setHeader(const std::string &key, const int &value) {
    std::ostringstream oss(value);
    _headers[key] = oss.str();
}

void	ResponseHeader::setContentType(std::string type, std::string path){
	type = path.substr(path.rfind(".") + 1, path.size() - path.rfind("."));
    _headers["Content-Type"] = _contentType[type];
    if ( _contentType[type].empty() ) _headers["Content-Type"] = "text/plain";
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
// Overloaders

ResponseHeader & ResponseHeader::operator=(const ResponseHeader & src){
	(void)src;
	return (*this);
}

		// Constructors and destructors

ResponseHeader::ResponseHeader(void){ resetHeaders(); }
ResponseHeader::ResponseHeader(const ResponseHeader & src){ (void)src; }}
ResponseHeader::~ResponseHeader(void){}
