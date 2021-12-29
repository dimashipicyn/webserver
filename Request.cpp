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

Request::Request() : state_(PARSE_FIRST_LINE) {
}

Request::~Request() {

}

void Request::parse(const char *buf)
{
    LOG_INFO("BUF: %s\n", buf);
    buffer_ << buf;
    std::size_t foundTwoNewLines = buffer_.str().find("\n\n");
    std::size_t foundTwoNewLinesWithCarriageReturn = buffer_.str().find("\r\n\r\n");
    if (foundTwoNewLines                      == std::string::npos
        && foundTwoNewLinesWithCarriageReturn == std::string::npos)
    {
        LOG_DEBUG("Query dont full\n");
        return;
    }
    // parse
    switch (state_) {
        case PARSE_FIRST_LINE:
            LOG_DEBUG("Parse first line\n");
            parse_first_line();
            //break;
        case PARSE_HEADERS:
            LOG_DEBUG("Parse headers\n");
            parse_headers();
            //break;
        case PARSE_BODY:
            LOG_DEBUG("Parse body\n");
            parse_body();
            //break;
        case PARSE_DONE:
            LOG_DEBUG("Parse done\n");
            break;
        case PARSE_ERROR:
            LOG_DEBUG("Parse error\n");
            break;
        default:
            LOG_DEBUG("Undefined enum value: %d\n", state_);
            break;
    }
}

static void skipnNewLines(std::stringstream& ss)
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
    buffer_ >> method_ >> path_ >> version_;
    skipnNewLines(buffer_);
    if (method_.empty() || path_.empty() || version_.empty()) {
        state_ = PARSE_ERROR;
    }
    else {
        // проверяем на наличие query string
        std::size_t found = path_.find('?');
        if (found != std::string::npos) {
            std::size_t queryStringLen = path_.size() - found;
            query_string_ = path_.substr(found, queryStringLen);
            path_.erase(found, queryStringLen);
        }
        state_ = PARSE_HEADERS;
    }
}

void Request::parse_headers()
{
    std::string key;
    std::string value;

    skipnNewLines(buffer_);
    while (!buffer_.eof()) {
        std::getline(buffer_, key, ':');
        std::getline(buffer_, value, '\n');
        headers_.insert(std::pair<std::string, std::string>(key, value));
        skipnNewLines(buffer_);
    }
    state_ = PARSE_BODY;
}

void Request::parse_body()
{
    skipnNewLines(buffer_);
    std::string bodyStr;
    std::getline(buffer_, bodyStr, '\0');

    body_.append(bodyStr);

    buffer_.str("");
    buffer_.clear();
    state_ = PARSE_DONE;
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

void Request::reset() {
    method_ = "";
    path_ = "";
    version_ = "";
    query_string_ = "";
    body_ = "";
    buffer_.str("");
    buffer_.clear();
    headers_.clear();
    state_ = PARSE_FIRST_LINE;
}
