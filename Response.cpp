//
// Created by Lajuana Bespin on 11/15/21.
//

#include "Response.h"
#include <unistd.h>

#include <sstream>
#include <fstream>
#include <string>
#include <iterator>

#include <string>
#include <sstream>
#include <fstream>

#include <iostream>

Response::Response() {
}

Response::~Response() {

}

const std::string& Response::getContent() const { return _output; }
