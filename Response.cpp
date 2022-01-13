//
// Created by Lajuana Bespin on 11/15/21.
//

#include "Response.h"
#include "ResponseHeader.hpp"
#include "settingsManager/SettingsManager.hpp"
#include <unistd.h>
#include <map>

#include <sstream>
#include <fstream>
#include <string>
#include <iterator>

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


Response::Response() {}

Response::~Response() {}

// Change: take from config root && default files && error files
std::string Response::getPath(const Request &request) const{
	std::string path;
	SettingsManager *settingsManager = SettingsManager::getInstance(); //get existed instance or something else discuss this with Ruslan
	Route *route = settingsManager->getLastServer()->findRouteByPath(request.getPath());
	path = "/Users/griddler/webserver/index.html";//route->getRoot() + route->getLocation() + route->getDefaultFiles().at(0);
	return path;
}

//Change: take from config errorfile paths
std::string Response::getErrorPath(const Request &request) const {
	return ".\\root\\www\\page404.html";
}

const std::string& Response::getContent() const {
	return _output;
}

void Response::setStatusCode(int code){ _header.setStatusCode(code); }

void Response::setHeaderField(const std::string &key, const std::string &value) {
	_header.setHeaderField(key, value);
}

void Response::setHeaderField(const std::string &key, int value) {
	std::ostringstream os;
	os << value;
	_header.setHeaderField(key, os.str() );
}

void Response::setContentType(const std::string& path) {
	std::string type = getExtension(path);
	std::string value;
	try {
		value = _contentType.at(type);
	} catch(const std::exception& e) {
		value = "text/plain";
	}
	setHeaderField("Content-Type", value);
}

std::string		Response::getHeader(){
	return	_header.getHeader();
}