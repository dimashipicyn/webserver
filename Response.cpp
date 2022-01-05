//
// Created by Lajuana Bespin on 11/15/21.
//

#include "Response.h"
#include "ResponseHeader.hpp"
#include <unistd.h>
#include <map>

#include <sstream>
#include <fstream>
#include <string>
#include <iterator>


std::map<std::string, void (Response::*)(const Request &, const RequestConfig &)>	Response::initMethods()
{
    std::map<std::string, void (Response::*)(const Request &, const RequestConfig &)> map;

    map["GET"] = &Response::methodGET;
    map["POST"] = &Response::methodPOST;
    map["DELETE"] = &Response::methodDELETE;
    return map;
}

std::map<std::string, void (Response::*)(const Request &, const RequestConfig &)> Response::_method = Response::initMethods();

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

Response::Response(const Request& request, const RequestConfig& requestConfig){
    _header.setDate();
    std::string method = request.getMethod();
    if ( _method.count(method) ) (this->*_method[method])(request, requestConfig);
    else if (_allMethods.count(method)) methodNotAllowed(request, requestConfig);
    else BadRequest();
}

void Response::methodGET(const Request& request, const RequestConfig &){
    int errorCode = 404;
    std::string content = "<h1>404 Not Found</h1>";
    std::ifstream f(".\\wwwroot\\index.html");

    if (f.good()){
        std::string str((std::istreambuf_iterator<char>(f)),
                        std::istreambuf_iterator<char>());
        content = str;
        errorCode = 200;
    }
    f.close();

    _header.setHeader("Content-Length", content.size());

    std::ostringstream oss(_header.getHeader(request));
    oss << content;
    _output = oss.str();
}

void Response::methodPOST(const Request& request, const RequestConfig &){}
void Response::methodDELETE(const Request& request, const RequestConfig &){}

void Response::methodNotAllowed(const Request& request, const RequestConfig &){
    _header.setCode(405);
    std::string strAllowMethods;
    for (std::map<std::string, void (Response::*)(const Request &, const RequestConfig &)>::iterator it = _method.begin();
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

const std::string& Response::getContent() const {
	return _output;
}
