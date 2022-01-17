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

void Response::reset() {
    content_ = "";
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

std::string		Response::getErrorPage(int code) {
	std::ostringstream os;
	os << "<!DOCTYPE html>";
	os << "<html>";
	os << "<head>";
	os << "<title>An Error Occurred: Not Found</title>";
	os << "<style>";
	os << "body { background-color: #fff; color: #222; font:16px"
		  "Arial, sans-serif; margin: 0; }";
	os << "</style>";
	os << "</head>";
	os << "<body>";
	os << "<h1>Oops! An Error Occurred</h1>";
	os << "<h2>The server returned a " << code << " Not Found</h2>";
	os << "<p>";
	os << "Something is broken. Sorry for any inconvenience caused.";
	os << "</p>";
	os << "</div>";
	os << "</body>";
	os << "</html>";
	return os.str();
}

std::map<std::string, std::string> Response::initContentType() {
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

std::map<std::string, std::string> Response::_contentType
		= Response::initContentType();

