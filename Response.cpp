//
// Created by Lajuana Bespin on 11/15/21.
//

#include "Response.h"
#include "ResponseHeader.hpp"
#include <unistd.h>

#include <sstream>
#include <fstream>
#include <string>
#include <iterator>


Response::Response() {
}

Response::~Response() {

}

const std::string& Response::getContent() const {
	return _output;
}

void Response::errorPage(const Request& request) {
	ResponseHeader header;

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

	std::ostringstream contentLength(content.size());
	header.setHeader("Content-Length", contentLength.str());

	std::ostringstream oss(header.getHeader(request));
	oss << content;
	_output = oss.str();
}
