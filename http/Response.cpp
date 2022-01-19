//
// Created by Lajuana Bespin on 11/15/21.
//

#include "Response.h"
#include <unistd.h>
#include "utils.h"
#include "SettingsManager.hpp"
#include "Route.hpp"
#include "Request.h"

Response::Response() {

}

Response::~Response() {

}

void Response::setContent(const std::string &s) {
    content_ = s;
}

const std::string& Response::getContent() {
    content_ = "HTTP/1.1 "
            + utils::to_string(statusCode_)
            + " " + reasonPhrase[statusCode_]
            + "\r\n";

    if (!header_.empty()) {
        content_ += header_
                + "\r\n"
                + body_;
    }

	return content_;
}

//Change: take from config errorfile paths
std::string Response::getErrorPath(const Request &request) const {
	return ".\\root\\www\\page404.html";
}

void Response::setStatusCode(int code){
	statusCode_ = code;
}

void Response::setContentType(const std::string& path) {
	std::string type = utils::getExtension(path);
	std::string value = "text/plain";
	if (!type.empty() ) {
		try {
			value = _contentType.at(type);
		} catch (const std::out_of_range &e) {}
	}
	header_ += "Content-Type: " + value + "\n";
}

std::string		Response::getHeader(){
	return	header_;
}

void		Response::buildErrorPage(int code, const Request& request) {
	body_ = "<!DOCTYPE html>";
	body_ += "<html>";
	body_ += "<head>";
	body_ += "<title>Error" + utils::to_string(code) + "</title>";
	body_ += "</head>";
	body_ += "<body>";
	body_ += "<h1>Oops! An Error Occurred</h1>";
	body_ += "<h2>The server returned a " + utils::to_string(code) + ". ";
	body_ += reasonPhrase[code] + "</h2>";
	body_ += "</body>";
	body_ += "</html>";

	statusCode_ = code;
	header_ += "Host: " + request.getHost() + "\r\n";
	header_ += "Content-Length: " + utils::to_string(body_.size()) + "\r\n";
	header_ += "Content-Type: text/html\r\n";
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
	setHeaderField("Host", request.getHost() );
	setHeaderField("Content-Length", os.str().size());
	setHeaderField("Content-Type", "text/html");
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
//=================part from Response Header====================================
void Response::setHeaderField(const std::string &key, const std::string &value) {
	header_ += key + ": " + value + "\r\n";
}

void Response::setHeaderField(const std::string &key, int value) {
	header_ += key + ": " + utils::to_string(value) + "\n";
}

void Response::setBody(const std::string &body) {
	body_ = body;
}

const std::string& Response::getBody(){
	return body_;
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
	std::map<int, std::string> reasonPhrase;
	reasonPhrase[100] = "Continue";
	reasonPhrase[200] = "OK";
	reasonPhrase[201] = "Created";
	reasonPhrase[204] = "No Content";
	reasonPhrase[400] = "Bad Request";
	reasonPhrase[403] = "Forbidden";
	reasonPhrase[404] = "Not Found";
	reasonPhrase[405] = "Method Not Allowed";
	reasonPhrase[413] = "Payload Too Large";
	reasonPhrase[500] = "Internal Server Error";
	return reasonPhrase;
}

std::map<int, std::string> Response::reasonPhrase = Response::initErrorMap();

