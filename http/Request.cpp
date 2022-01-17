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
    : state_(PARSE_QUERY)
    , receivedBytes_(0)
    , buffer_()
    , host_()
    , port_()
    , method_()
    , version_()
    , path_()
    , query_string_()
    , body_()
    , headers_()
{
}

Request::~Request() {
}


Request::Request(const Request& request)
    : state_(request.state_)
    , receivedBytes_(request.receivedBytes_)
    , buffer_()
    , host_(request.host_)
    , port_(request.port_)
    , method_(request.method_)
    , version_(request.version_)
    , path_(request.path_)
    , query_string_(request.query_string_)
    , body_(request.body_)
    , headers_(request.headers_)
{
}

Request& Request::operator=(const Request& request) {
    if (this == &request) {
        return *this;
    }
    state_ = request.state_;
    receivedBytes_ = request.receivedBytes_;
    host_ = request.host_;
    port_ = request.port_;
    method_ = request.method_;
    version_ = request.version_;
    path_ = request.path_;
    query_string_ = request.query_string_;
    body_ = request.body_;
    headers_ = request.headers_;
    return *this;
}

void Request::parse(const char *buf)
{
    buffer_.write(buf, strlen(buf));

    std::cout << buffer_.str() << "\n";
    std::size_t foundTwoNewLines = buffer_.str().find("\n\n");
    std::size_t foundTwoNewLinesWithCarriageReturn = buffer_.str().find("\r\n\r\n");
    if (   (state_ == PARSE_QUERY)
        && (foundTwoNewLines                   == std::string::npos)
        && (foundTwoNewLinesWithCarriageReturn == std::string::npos)
        )
    {
        LOG_DEBUG("Query no two new lines\n");
        return;
    }

    // parse
    if (state_ == PARSE_QUERY) {
        LOG_DEBUG("Parse query\n");
        parse_first_line();
        parse_headers();

        if (hasHeader("Content-Length")) {
            state_ = PARSE_BODY_WITH_LENGTH;
        }
        else if (hasHeader("Transfer-Encoding") && getHeaderValue("Transfer-Encoding") == "chunked") {
            state_ = PARSE_CHUNKED_BODY;
        }
        else {
            state_ = PARSE_BODY;
        }
    }
    if (state_ == PARSE_BODY) {
        LOG_DEBUG("Parse body\n");
        parse_body();
    }
    if (state_ == PARSE_BODY_WITH_LENGTH) {
        LOG_DEBUG("Parse body with length\n");
        parse_body_with_length();
    }
    if (state_ == PARSE_CHUNKED_BODY) {
        LOG_DEBUG("Parse chunked body\n");
        parse_chunked_body();
    }
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
    std::string line;
    std::getline(buffer_, line, '\n');

    std::stringstream firstLine(line);

    firstLine >> method_ >> path_ >> version_;
    if (method_.empty() || path_.empty() || version_.empty()) {
        state_ = PARSE_ERROR;
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
    std::string line;

    std::getline(buffer_, line, '\n');
    while (!line.empty())
    {
        if (utils::isValidPairString(line, ':')) {
            headers_.insert(utils::breakPair(line, ':'));
        }
        else {
            state_ = PARSE_ERROR;
            return;
        }
        std::getline(buffer_, line, '\n');
    }
}

void Request::parse_body_with_length()
{
    uint64_t contentLen = utils::to_number<uint64_t>(getHeaderValue("Content-Length"));

    std::string line;
    std::getline(buffer_, line, '\0');

    if ((receivedBytes_ + line.size()) <= contentLen) {
        receivedBytes_ += line.size();
        body_.append(line);
    }
    else {
        int64_t size = contentLen - receivedBytes_;
        receivedBytes_ = contentLen;
        body_.append(line, 0, size);
        buffer_.str(std::string(line, size, line.size()));
    }


    if (buffer_.eof()) {
        buffer_.str("");
        buffer_.clear();
    }


    if (contentLen == receivedBytes_) {
        state_ = PARSE_DONE;
    }
}

void Request::parse_chunked_body() {

}

void Request::parse_body() {
    std::string s;
    std::getline(buffer_, s, '\0');
    body_.append(s);
    buffer_.str("");
    buffer_.clear();
}

Request::State Request::getState() const {
    return state_;
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

std::string Request::drainBody()
{
    std::string res(body_);
    body_.clear();
    return res;
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

bool Request::hasHeader(const std::string &key) {
    if (headers_.find(key) != headers_.end()) {
        return true;
    }
    return false;
}

const std::string& Request::getHeaderValue(const std::string &key) {
    return headers_[key];
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
    receivedBytes_ = 0;
    host_.clear();
    port_.clear();
    state_ = PARSE_QUERY;
}
