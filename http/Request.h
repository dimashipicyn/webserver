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

public:
    Request();
    ~Request();
    Request(const Request& request);
    Request& operator=(const Request& request);

    void setHost(const std::string& host);
    void setID(int id);

    const std::string&      getMethod() const;
    const std::string&      getVersion() const;
    const std::string&      getPath() const;
    const std::string&      getQueryString() const;
    const std::string&      getBody() const;

	void setBody(const std::string &body);

	const std::string&      getHost() const;
    const headersMap&       getHeaders() const;
    int                     getID() const;

    bool                    hasHeader(const std::string& key) const;
    const std::string&      getHeaderValue(const std::string& key) const;

    bool good() const;

    void parse(const std::string& s);
    void reset();


private:
    void parse_first_line();
    void parse_headers();
    void parse_body();

private:
    std::stringstream   buffer_;
    std::string         host_;
    std::string         method_;
    std::string         version_;
    std::string         path_;
    std::string         query_string_;
    std::string         body_;
    headersMap          headers_;
    int                 id_;
    bool                isGood_;

};

std::ostream& operator<<(std::ostream& os, const Request& request);

#endif //WEBSERV_REQUEST_H
