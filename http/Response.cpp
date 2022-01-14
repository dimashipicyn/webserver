//
// Created by Lajuana Bespin on 11/15/21.
//

#include "Response.h"
#include <unistd.h>

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