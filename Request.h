//
// Created by Lajuana Bespin on 11/15/21.
//

#ifndef WEBSERV_REQUEST_H
#define WEBSERV_REQUEST_H

#include <map>
#include <string>
#include <sstream>

class Request {
public:
    typedef std::map<std::string, std::string> headersMap;

    enum State {
        PARSE_FIRST_LINE,
        PARSE_HEADERS,
        PARSE_BODY,
        PARSE_ERROR,
        PARSE_DONE
    };

public:
    Request();
    ~Request();

    void parse(const char* buf);

    void parse_first_line();
    void parse_headers();
    void parse_body();

    const std::string&      getMethod() const;
    const std::string&      getVersion() const;
    const std::string&      getPath() const;
    const std::string&      getQueryString() const;
    const std::string&      getBody() const;
    const headersMap&       getHeaders() const;
    State                   getState() const;



    void reset();

private:
    Request(const Request&) {};
    Request& operator=(const Request&) {return *this;};

private:
    State               state_;
    std::stringstream   buffer_;
    std::string         method_;
    std::string         version_;
    std::string         path_;
    std::string         query_string_;
    std::string         body_;
    headersMap          headers_;
};

std::ostream& operator<<(std::ostream& os, const Request& request);

#endif //WEBSERV_REQUEST_H
