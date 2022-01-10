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

Request::Request() : state_(PARSE_QUERY) {
}

Request::~Request() {
}

void Request::parse(const char *buf)
{
    buffer_.write(buf, strlen(buf));

    std::size_t foundTwoNewLines = buffer_.str().find("\n\n");
    std::size_t foundTwoNewLinesWithCarriageReturn = buffer_.str().find("\r\n\r\n");
    if (   (state_ == PARSE_QUERY)
        && (foundTwoNewLines                   == std::string::npos)
        && (foundTwoNewLinesWithCarriageReturn == std::string::npos)
        )
    {
        LOG_DEBUG("Query dont full\n");
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
        else if (   hasHeader("Transfer-Encoding")
            && getHeaderValue("Transfer-Encoding") == "chunked"){
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
    buffer_ >> method_ >> path_ >> version_;
    skipNewLines(buffer_);
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
    std::string key;
    std::string value;

    skipNewLines(buffer_);
    while (!buffer_.eof()) {
        std::getline(buffer_, key, ':');
        std::getline(buffer_, value, '\n');
        headers_.insert(std::pair<std::string, std::string>(utils::trim(key, " "), value));
        skipNewLines(buffer_);
    }
}

void Request::parse_body_with_length()
{
    uint64_t contentLen = utils::to_number<uint64_t>(getHeaderValue("Content-Length"));
    const uint64_t bufSize = 1 << 16;

    skipNewLines(buffer_);

    std::streamsize readLen = static_cast<std::streamsize>(std::min(contentLen - body_.size(), bufSize));
    std::string bodyStr;
    bodyStr.resize(readLen + 1); // для нуль терминатора
    buffer_.read(&bodyStr[0], readLen);

    body_.append(bodyStr.c_str());
    buffer_.str("");
    buffer_.clear();

    if (contentLen == body_.size()) {
        state_ = PARSE_DONE;
    }
}

void Request::parse_chunked_body() {

}

void Request::parse_body() {
    if (buffer_.str().empty()) {
        state_ = PARSE_DONE;
        return;
    }
    body_.append(buffer_.str());
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
    method_ = "";
    path_ = "";
    version_ = "";
    query_string_ = "";
    body_ = "";
    buffer_.str("");
    buffer_.clear();
    headers_.clear();
    state_ = PARSE_QUERY;
}
