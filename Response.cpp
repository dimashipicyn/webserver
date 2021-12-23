//
// Created by Lajuana Bespin on 11/15/21.
//

#include "Response.h"
#include <unistd.h>
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>

Response::Response() {

}

Response::~Response() {
}

void Response::write(int fd) {
	std::ifstream f(".\\wwwroot\\index.html");
	std::string content = "<h1>404 Not Found</h1>";
	int errorCode = 404;
	if (f.good()){
		std::string str ((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
		content = str;
		errorCode = 200;
	}
	f.close();
	std::ostringstream oss;
	oss << "HTTP/1.1 " << errorCode << " OK\r\n";
	oss << "Cache-Control: no-cache, private\r\n";
	oss << "Content-Type: text/html\r\n";
	oss << "Content-Length: " << content.size() << "\r\n";
	oss << "\r\n";
	oss << content;

		std::string output = oss.str();
		int size = output.size() + 1;


	::write(fd, content.c_str(), size);
}
