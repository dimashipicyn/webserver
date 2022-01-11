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

extern SettingsManager *settingsManager;

std::map<std::string, void (Response::*)(const Request &)>	Response::initMethods()
{
    std::map<std::string, void (Response::*)(const Request &)> map;

    map["GET"] = &Response::methodGET;
    map["POST"] = &Response::methodPOST;
    map["DELETE"] = &Response::methodDELETE;
    return map;
}

std::map<std::string, void (Response::*)(const Request &)> Response::_method = Response::initMethods();

std::set<std::string> Response::initAllMethods(){
    std::set<std::string> allMethods;
    allMethods.insert("GET");
    allMethods.insert("HEAD");
    allMethods.insert("POST");
    allMethods.insert("PUT");
    allMethods.insert("DELETE");
    allMethods.insert("CONNECT");
    allMethods.insert("OPTIONS");
    allMethods.insert("TRACE");
    allMethods.insert("PATCH");
    return allMethods;
}

std::set<std::string> Response::_allMethods = initAllMethods();

Response::Response() {}

Response::~Response() {}

Response::Response(const Request& request){
    _header.setHeader("Date", _header.getDate());
    _header.setHeader("Host", settingsManager->getLastServer()->getHost());
    std::string method = request.getMethod();
    if ( _method.count(method) ) (this->*_method[method])(request);
    else if (_allMethods.count(method)) methodNotAllowed(request);
    else BadRequest();
}

void Response::methodGET(const Request& request){
    int errorCode = 404;
    std::string content = "<h1>404 Not Found</h1>";
    std::string path = getPath(request);
    std::ifstream f(path);
    if (f.good()) errorCode = 200;
	else{
		path = getErrorPath();
		std::ifstream f(path);
	}
	std::string str((std::istreambuf_iterator<char>(f)),
					std::istreambuf_iterator<char>());
	content = str;
	f.close();
	_header.setHeader("Content-Length", content.size());

	std::ostringstream oss(_header.getHeader(request));
	oss << content;
	_output = oss.str();
}

void Response::methodPOST(const Request& request){}
void Response::methodDELETE(const Request& request){}

void Response::methodNotAllowed(const Request& request){
    _header.setCode(405);
    std::string strAllowMethods;
    for (std::map<std::string, void (Response::*)(const Request &)>::iterator it = _method.begin();
            it != _method.end(); ++it){
        if (it == _method.begin()) strAllowMethods += it->first;
        else{
            strAllowMethods += ", ";
            strAllowMethods += it->first;
        }
    }
    _header.setHeader("Allow", strAllowMethods);
}

void Response::BadRequest(){
    _header.setCode(400);

}
// Change: take from config root && default files && error files
std::string Response::getPath(const Request &request) const{
	std::string path;
	Route *route = settingsManager->getLastServer()->findRouteByPath(request.getPath());
	path = route->getRoot() + route->getLocation() + route->getDefaultFiles().at(0);
	return path;
}

//Change: take from config errorfile paths
std::string Response::getErrorPath() {

	return ".\\root\\www\\page404.html";
}

const std::string& Response::getContent() const {
	return _output;
}
