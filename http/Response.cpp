//
// Created by Lajuana Bespin on 11/15/21.
//

#include "Response.h"
#include <unistd.h>
#include "utils.h"
#include "SettingsManager.hpp"
#include "Route.hpp"

Response::Response() {

}

Response::~Response() {

}

void Response::setContent(const std::string &s) {
    content_ = s;
}

const std::string& Response::getContent() {
    return content_;
}

//Change: take from config errorfile paths
std::string Response::getErrorPath(const Request &request) const {
	return ".\\root\\www\\page404.html";
}

void Response::setStatusCode(int code){ header_.setStatusCode(code); }

void Response::setHeaderField(const std::string &key, const std::string &value) {
	header_.setHeaderField(key, value);
}

void Response::setHeaderField(const std::string &key, int value) {
	std::ostringstream os;
	os << value;
	header_.setHeaderField(key, os.str() );
}

void Response::setContentType(const std::string& path) {
	std::string type = utils::getExtension(path);
	std::string value;
	try {
		value = _contentType.at(type);
	} catch(const std::exception& e) {
		value = "text/plain";
	}
	setHeaderField("Content-Type", value);
}

std::string		Response::getHeader(){
	return	header_.getHeader();
}

void		Response::buildErrorPage(int code, const Request& request) {
	std::ostringstream os;
	os << "<!DOCTYPE html>";
	os << "<html>";
	os << "<head>";
	os << "<title>Error" << code << "</title>";
	os << "</head>";
	os << "<body>";
	os << "<h1>Oops! An Error Occurred</h1>";
	os << "<h2>The server returned a " << code << ". " << _errors[code] << "</h2>";
	os << "</body>";
	os << "</html>";

	setStatusCode(code);
	setHeaderField("Host", request.getHost());
	setHeaderField("Content-Length", os.str().size());
	setHeaderField("Content-Type", "text/html");
	content_ = getHeader() + os.str();
}

void Response::buildDelPage(const Request& request) {
	std::ostringstream os;
	os << "<!DOCTYPE html>";
	os << "<html>";
	os << "<body>";
	os << "<h1>File deleted.</h1>";
	os << "</body>";
	os << "/html";
	setStatusCode(200);
	setHeaderField("Host", request.getHost());
	setHeaderField("Content-Length", os.str().size());
	setHeaderField("Content-Type", "text/html");
	content_ = getHeader() + os.str();
}

std::string Response::readFile(const std::string &path) {
	std::ifstream sourceFile;
	sourceFile.open(path.c_str(), std::ifstream::in);
	std::ostringstream os;
	if (!sourceFile.is_open()) throw httpEx<NotFound>("Cannot open requested resource");
	os << sourceFile.rdbuf();
	sourceFile.close();
	return os.str();
}

void    Response::writeFile(const std::string& path, const std::string body){
    std::ofstream	file;
    file.open(path.c_str(), std::ofstream::out | std::ofstream::trunc);
    if (!file.is_open()) throw httpEx<Forbidden>("Forbidden");
    file << body;
    file.close();
}

void	Response::writeContent(const std::string& path, const Request& request) {
	std::ofstream	file;
    std::string body = request.getBody();
	if (utils::isFile(path)) {
		file.open(path.c_str());
		file << body;
		file.close();
        body.clear();
		setStatusCode(204);
	}
	else {
		file.open(path.c_str(), std::ofstream::out | std::ofstream::trunc);
		if (!file.is_open()) throw httpEx<Forbidden>("Forbidden");
		file << body;
		file.close();
		setStatusCode(201);
	}
    setHeaderField("Content-Type", request.getHeaders().at("Content-Type"));
    setHeaderField("Content-Length", body.size());
    setHeaderField("Content-Location", path);
    content_ = getHeader() + body;
}

std::map<std::string, std::string> Response::initContentType() {
	std::map<std::string, std::string> contentType;

	contentType[".html"] = "text/html";
	contentType[".css"] = "text/css";
	contentType[".js"] = "text/javascript";
	contentType[".jpeg"] = "image/jpeg";
	contentType[".jpg"] = "image/jpeg";
	contentType[".png"] = "image/png";
	contentType[".bmp"] = "image/bmp";
	contentType[".txt"] = "text/plain";
	return contentType;
}

std::map<std::string, std::string> Response::_contentType
		= Response::initContentType();

std::map<int, std::string>	Response::initErrorMap(){
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

std::map<int, std::string> Response::_errors
		= ResponseHeader::initErrorMap();

