//
// Created by Griselda Riddler on 1/14/22.
//

#ifndef WEBSERV_SERVERERROR_HPP
#define WEBSERV_SERVERERROR_HPP

#include <exception>

class ServerError : public std::exception{
	const char * _s;
public:
	ServerError(const char *);
	const char * what() const throw();
};


#endif //WEBSERV_SERVERERROR_HPP
