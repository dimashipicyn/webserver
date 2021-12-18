//
// Created by Lajuana Bespin on 11/15/21.
//

#include "Response.h"
#include <unistd.h>

Response::Response() {

}

Response::~Response() {

}

void Response::write(int fd) {
    char *s = "HTTP/1.1 200 OK\n\n";
    ::write(fd, s, strlen(s));
}
