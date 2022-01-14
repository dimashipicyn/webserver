//
// Created by Griselda Riddler on 1/14/22.
//

#include "ServerError.hpp"

ServerError::ServerError(const char *s) : _s(s){}

const char *ServerError::what() const throw() {
	return _s;
}
