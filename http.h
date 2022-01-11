#ifndef HTTP_H
#define HTTP_H

#include <string>
#include <set>
#include "TcpSocket.h"
#include "EventPool.h"

#include "Response.h"
#include "ResponseHeader.hpp"
#include "settingsManager/SettingsManager.hpp"
#include "cgi/Cgi.hpp"



class Request;
class Response;

class HTTP
{
public:
    HTTP(const std::string& host, std::int16_t port);
    virtual ~HTTP();

    void handler(Request& request);
    void start();

	Response    response_;
private:
    EventPool   evPool_;

};

#endif // HTTP_H
