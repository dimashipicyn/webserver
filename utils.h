//
// Created by Lajuana Bespin on 11/15/21.
//

#ifndef WEBSERV_UTILS_H
#define WEBSERV_UTILS_H

namespace utils {
    int                         get_next_line(int fd, std::string& line);
    std::vector<std::string>    split(std::string& s, const char delim);
    std::string                 trim(const std::string &str, const std::string &whitespace);
    std::string                 to_string(int64_t num);
    std::string                 to_string(double num);
    int64_t                     to_int64(const std::string& stringToNumber);
    double                      to_double(const std::string& stringToNumber);
}
#endif //WEBSERV_UTILS_H
