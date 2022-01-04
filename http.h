#ifndef HTTP_H
#define HTTP_H

#include <string>
#include <map>
#include "TcpSocket.h"
#include "EventPool.h"
#include "settingsManager/SettingsManager.hpp"
#include "cgi/Cgi.hpp"


class Request;
class Response;

class HTTP
{
public:
    HTTP(const std::string& host, std::int16_t port);
    virtual ~HTTP();

    void handler(Request& request, Response &response);
    void start();

private:
    EventPool   evPool_;
};

#endif // HTTP_H
