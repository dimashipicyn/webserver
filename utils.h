//
// Created by Lajuana Bespin on 11/15/21.
//

#ifndef WEBSERV_UTILS_H
#define WEBSERV_UTILS_H

int	get_next_line(int fd, std::string& line);
std::vector<std::string>    split(std::string& s, const char delim);
std::string trim(const std::string &str, const std::string &whitespace);

#endif //WEBSERV_UTILS_H
