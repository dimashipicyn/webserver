//
// Created by Lajuana Bespin on 11/15/21.
//

#include <fstream>
#include <iostream>
#include <vector>
#include <unistd.h>
#include "Request.h"
#include "utils.h"
#include "Logger.h"

Request::Request()
    : buffer_()
    , host_()
    , method_()
    , version_()
    , path_()
    //, query_string_()
    , body_()
    , headers_()
    , id_(-1)
    , isGood_(true)
{
}

Request::~Request() {
}


Request::Request(const Request& request)
    : buffer_()
    , host_(request.host_)
    , method_(request.method_)
    , version_(request.version_)
    , path_(request.path_)
    , query_string_(request.query_string_)
    //, body_(request.body_)
    , headers_(request.headers_)
    , id_(-1)
    , isGood_(true)
{
}

Request& Request::operator=(const Request& request) {
    if (this == &request) {
        return *this;
    }
    host_ = request.host_;
    method_ = request.method_;
    version_ = request.version_;
    path_ = request.path_;
    query_string_ = request.query_string_;
    body_ = request.body_;
    headers_ = request.headers_;
    id_ = request.id_;
    isGood_ = request.isGood_;
    return *this;
}

void Request::parse(const std::string& s)
{
    buffer_.write(s.c_str(), s.size());

    LOG_DEBUG("Parse query\n");

    // parse
    parse_first_line();
    parse_headers();
    parse_body();
}

static void skipNewLines(std::stringstream& ss)
{
    char ch = '\0';
    while (!ss.eof()) {
        ss >> ch;
        if (ch == '\0')
            return;
        if (ch != '\n' && ch != '\r') {
            ss.putback(ch);
            return;
        }
    }
}

void Request::parse_first_line()
{
    skipNewLines(buffer_);
    std::string line;
    std::getline(buffer_, line, '\n');

    std::stringstream firstLine(line);

    firstLine >> method_ >> path_ >> version_;
    if (method_.empty() || path_.empty() || version_.empty()) {
        isGood_ = false;
    }
    else {
        // проверяем на наличие query string
        std::size_t found = path_.find('?');
        if (found != std::string::npos) {
            std::size_t queryStringLen = path_.size() - found - 1;
            query_string_ = path_.substr(found + 1, queryStringLen);
            path_.erase(found, queryStringLen + 1);
        }
    }
}

void Request::parse_headers()
{
    if (!isGood_) { // false
        return;
    }

    std::string line;
    size_t pos;

    std::getline(buffer_, line, '\n');
    if ((pos = line.find_last_of("\r")) != std::string::npos) {
        line.erase(pos);
    }
    while (!line.empty())
    {
        pos = line.find_first_of(":");
        if (pos == std::string::npos) {
            isGood_ = false;
            return;
        }

        headers_[line.substr(0, pos)] = utils::trim(line.substr(pos + 1), " \t\n\r");

        std::getline(buffer_, line, '\n');
        if ((pos = line.find_last_of("\r")) != std::string::npos) {
            line.erase(pos);
        }
    }
}

void Request::parse_body() {
    if (!isGood_) { // false
        return;
    }
    std::string s;
    std::getline(buffer_, s, '\0');
    body_.append(s);
    buffer_.str("");
    buffer_.clear();
}

std::ostream& operator<<(std::ostream& os, const Request& request) {
    os << request.getMethod() << " "
    << request.getPath() << " "
    << request.getVersion() << std::endl;
    const std::map<std::string, std::string>& headers = request.getHeaders();
    for (std::map<std::string, std::string>::const_iterator begin = headers.begin(); begin != headers.end(); begin++) {
        os << "Key: " << begin->first << " - Value: " << begin->second << std::endl;
    }
    os << "Query string: " << request.getQueryString() << std::endl;
    os << "Body: " << request.getBody() << std::endl;
    return os;
}

const std::string& Request::getQueryString() const
{
    return query_string_;
}

const std::string& Request::getBody() const
{
    return body_;
}

const Request::headersMap& Request::getHeaders() const
{
    return headers_;
}

const std::string &Request::getMethod() const {
    return method_;
}

const std::string &Request::getVersion() const {
    return version_;
}

const std::string &Request::getPath() const {
    return path_;
}

const std::string &Request::getHost() const {
    return host_;
}

void Request::setHost(const std::string& host) {
    host_ = host;
}

bool Request::hasHeader(const std::string &key) const {
    if (headers_.find(key) != headers_.end()) {
        return true;
    }
    return false;
}

const std::string& Request::getHeaderValue(const std::string &key) const {
    if (headers_.find(key) != headers_.end()) {
        return headers_.find(key)->second;
    }
    throw std::runtime_error("NOT FOUND HEADER VALUE\n");
}

void Request::reset() {
    method_.clear();
    path_.clear();
    version_.clear();
    query_string_.clear();
    body_.clear();
    buffer_.str("");
    buffer_.clear();
    headers_.clear();
    body_.clear();
    isGood_ = true;
}

bool Request::good() const {
    return isGood_;
}

void Request::setID(int id) {
    id_ = id;
}

int Request::getID() const {
    return id_;
}

void Request::setBody(const std::string &body)
{
	body_ = body;
}
