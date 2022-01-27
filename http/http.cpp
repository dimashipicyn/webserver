#include <iostream>
#include <array>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>


#include "Logger.h"
#include "http.h"
#include "Request.h"
#include "Response.h"
#include "Autoindex.hpp"
#include "Route.hpp"
#include "Server.hpp"
#include "utils.h"
#include "TcpSocket.h"
#include "SettingsManager.hpp"
#include "Cgi.hpp"
#include "httpExceptions.h"
#include <algorithm>


///////////////////////////////////////////////////////////////////////////
///////////////////////// HTTP LOGIC //////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

std::map<std::string, std::string> parse_headers(const std::string& s)
{
    std::stringstream ss(s);
    std::map<std::string, std::string> headers;
    std::string line;
    size_t pos;

    std::getline(ss, line, '\n');
    if ((pos = line.find_last_of("\r")) != std::string::npos) {
        line.erase(pos);
    }
    while (!line.empty())
    {
        pos = line.find_first_of(":");
        if (pos == std::string::npos) {
            continue;
        }

        headers[line.substr(0, pos)] = utils::trim(line.substr(pos + 1), " \t\n\r");

        std::getline(ss, line, '\n');
        if ((pos = line.find_last_of("\r")) != std::string::npos) {
            line.erase(pos);
        }
    }
    return headers;
}
ssize_t writeFromBuf(int fd, std::string& wBuf, size_t nBytes);
ssize_t readToBuf(int fd, std::string& rBuf);

bool HTTP::cgi(const Request &request, Response& response, Route* route) {
    std::string path = request.getPath();
	size_t cgiAt = utils::checkCgiExtension(path, route->getCgi());
    bool isCGI = route != nullptr && cgiAt != std::string::npos;
    if (isCGI) {
		path = path.substr(0, cgiAt);
//        if (!utils::isFile(route->getFullPath(path)))
//            throw httpEx<NotFound>("CGI script not found");

        std::string ret = Cgi(request, *route).runCGI();
        size_t pos = ret.find_first_not_of("\r\n", ret.find("\r\n\r\n"));

        std::string size;
        if (pos != std::string::npos) {
            size = utils::to_string<size_t>(ret.size() - pos);
        }
        else {
            size = "0";
        }
		interpretResponseString(ret, response);
    }
    return isCGI;
}

bool HTTP::autoindex(const Request &request, Response &response, Route *route) {
    bool isAutoindexed = false;
    if (route != nullptr && route->isAutoindex()) {
        try {
            route->getDefaultFileName(request.getPath());
        } catch (Route::DefaultFileNotFoundException &e)
        {
            std::string html = Autoindex(*route).generatePage(request.getPath());
            response.setHeaderField("Content-Type", "text/html");
            response.setBody(html);
            isAutoindexed = true;
        }
    }
    return isAutoindexed;
}

int HTTP::redirection(const std::string& from, std::string &to, Route* route) {
	int statusCode = route->checkRedirectOnPath(to, from);
	return statusCode;
}

void HTTP::methodsCaller(Request& request, Response& response, Route* route) {
    try {
        checkIfAllowed(request.getMethod(), route);
        (this->*_method.at(request.getMethod()))(request, response, route);
    }
    catch (const std::out_of_range& e) {
        throw httpEx<BadRequest>("Invalid Request");
    }
}


// здесь происходит обработка запроса
void HTTP::handler(Request& request, Response& response) {
    Server *server = SettingsManager::getInstance()->findServer(request.getHost());
	Route *route = (server == nullptr ? nullptr : server->findRouteByPath(request.getPath()));

    try {
        LOG_DEBUG("Http handler call\n");
        LOG_DEBUG("--------------PRINT REQUEST--------------\n");

		std::ostringstream headers;
		for (Request::headersMap::const_iterator i = request.getHeaders().begin(); i != request.getHeaders().end(); i++) {
			headers << (*i).first.c_str() << ": " << (*i).second.c_str() << std::endl;
		}
		LOG_INFO("Request Data:\n%s %s\n%s\n", request.getMethod().c_str(), request.getPath().c_str(), headers.str()
		.c_str());

		if (request.getBody().size() > route->getMaxBodySize())
			request.setBody("");

        if (route == nullptr) {
            throw httpEx<NotFound>("Not Found");
        }

        /* Request check supported version, check valid path, check empty */
        requestValidate(request);

        if (cgi(request, response, route)) {
            return;
        }

        methodsCaller(request, response, route);

    }
    catch (httpEx<BadRequest> &e) {
        LOG_INFO("BadRequest: %s\n", e.what());
        int size = response.buildErrorPage(e.error_code, server->getErrorPage());
        response.setStatusCode(e.error_code);
        response.setHeaderField("Host", request.getHost());
        response.setHeaderField("Content-Type", "text/html");
        response.setHeaderField("Content-Length", size);
    }
    catch (httpEx<Unauthorized> &e) {
        LOG_INFO("Unauthorized: %s\n", e.what());
    }
    catch (httpEx<PaymentRequired> &e) {
        LOG_INFO("PaymentRequired: %s\n", e.what());
    }
    catch (httpEx<Forbidden> &e) {
        LOG_INFO("Forbidden: %s\n", e.what());
		int size = response.buildErrorPage(e.error_code, server->getErrorPage());
        response.setStatusCode(e.error_code); // одинаковые для всех элементы можно пихнуть в эррор билдер?
        response.setHeaderField("Host", request.getHost());
        response.setHeaderField("Content-Type", "text/html");
        response.setHeaderField("Content-Length", size);
    }
    catch (httpEx<NotFound> &e) {
        LOG_INFO("NotFound: %s\n", e.what());
		int size = response.buildErrorPage(e.error_code, server->getErrorPage());
        response.setStatusCode(e.error_code); // одинаковые для всех элементы можно пихнуть в эррор билдер?
        response.setHeaderField("Host", request.getHost());
        response.setHeaderField("Content-Type", "text/html");
		response.setHeaderField("Content-Length", size);
    }
    catch (httpEx<MethodNotAllowed> &e) {
        LOG_INFO("MethodAllowed: %s\n", e.what());
        int size = response.buildErrorPage(e.error_code, server->getErrorPage());
        response.setStatusCode(e.error_code);
        response.setHeaderField("Host", request.getHost());
        response.setHeaderField("Content-Type", "text/html");
        response.setHeaderField("Content-Length", size);
        response.setHeaderField("Allow", e.what());
    }
    catch (httpEx<NotAcceptable> &e) {
        LOG_INFO("NotAcceptable: %s\n", e.what());
    }
    catch (httpEx<ProxyAuthenticationRequired> &e) {
        LOG_INFO("ProxyAuthenticationRequired: %s\n", e.what());
    }
    catch (httpEx<RequestTimeout> &e) {
        LOG_INFO("RequestTimeout: %s\n", e.what());
    }
    catch (httpEx<Conflict> &e) {
        LOG_INFO("Conflict: %s\n", e.what());
    }
    catch (httpEx<Gone> &e) {
        LOG_INFO("Gone: %s\n", e.what());
    }
    catch (httpEx<LengthRequired> &e) {
        LOG_INFO("LengthRequired: %s\n", e.what());
    }
    catch (httpEx<PreconditionFailed> &e) {
        LOG_INFO("PreconditionFailed: %s\n", e.what());
    }
    catch (httpEx<PayloadToLarge> &e) {
        LOG_INFO("PayloadToLarge: %s\n", e.what());
    }
    catch (httpEx<UriTooLong> &e) {
        LOG_INFO("UriTooLong: %s\n", e.what());
    }
    catch (httpEx<UnsupportedMediaType> &e) {
        LOG_INFO("UnsupportedMediaType: %s\n", e.what());
    }
    catch (httpEx<RangeNotSatisfiable> &e) {
        LOG_INFO("RangeNotSatisfiable: %s\n", e.what());
    }
    catch (httpEx<ExpectationFailed> &e) {
        LOG_INFO("ExpectationFailed: %s\n", e.what());
    }
    catch (httpEx<ImATeapot> &e) {
        LOG_INFO("ImATeapot: %s\n", e.what());
    }
    catch (httpEx<UnprocessableEntity> &e) {
        LOG_INFO("UnprocessableEntity: %s\n", e.what());
    }
    catch (httpEx<TooEarly> &e) {
        LOG_INFO("TooEarly: %s\n", e.what());
    }
    catch (httpEx<UpgradeRequired> &e) {
        LOG_INFO("UpgradeRequired: %s\n", e.what());
    }
    catch (httpEx<PreconditionRequired> &e) {
        LOG_INFO("PreconditionRequired: %s\n", e.what());
    }
    catch (httpEx<TooManyRequests> &e) {
        LOG_INFO("TooManyRequests: %s\n", e.what());
    }
    catch (httpEx<RequestHeaderFieldsTooLarge> &e) {
        LOG_INFO("RequestHeaderFieldsTooLarge: %s\n", e.what());
    }
    catch (httpEx<UnavailableForLegalReasons> &e) {
        LOG_INFO("UnavailableForLegalReasons: %s\n", e.what());
    }
    catch (httpEx<InternalServerError> &e) {
        LOG_INFO("InternalServerError: %s\n", e.what());
		int size = response.buildErrorPage(e.error_code, server->getErrorPage());
		response.setStatusCode(e.error_code);
		response.setHeaderField("Host", request.getHost());
		response.setHeaderField("Content-Type", "text/plain");
		response.setHeaderField("Content-Length", size);
    }
    catch (httpEx<NotImplemented> &e) {
        LOG_INFO("NotImplemented: %s\n", e.what());
    }
    catch (httpEx<BadGateway> &e) {
        LOG_INFO("BadGateway: %s\n", e.what());
    }
    catch (httpEx<ServiceUnavailable> &e) {
        LOG_INFO("ServiceUnavailable: %s\n", e.what());
    }
    catch (httpEx<GatewayTimeout> &e) {
        LOG_INFO("GatewayTimeout: %s\n", e.what());
    }
    catch (httpEx<HTTPVersionNotSupported> &e) {
        LOG_INFO("HTTPVersionNotSupported: %s\n", e.what());
    }
    catch (httpEx<VariantAlsoNegotiates> &e) {
        LOG_INFO("VariantAlsoNegotiates: %s\n", e.what());
    }
    catch (httpEx<InsufficientStorage> &e) {
        LOG_INFO("InsufficientStorage: %s\n", e.what());
    }
    catch (httpEx<LoopDetected> &e) {
        LOG_INFO("LoopDetected: %s\n", e.what());
    }
    catch (httpEx<NotExtended> &e) {
        LOG_INFO("NotExtended: %s\n", e.what());
    }
    catch (httpEx<NetworkAuthenticationRequired> &e) {
        LOG_INFO("NetworkAuthenticationRequired: %s\n", e.what());
    }
}

//=======================Moved from web.1.0 ===================================

void HTTP::methodGET(const Request& request, Response& response, Route* route){
    LOG_DEBUG("Http handler call\n");
    LOG_DEBUG("--------------PRINT REQUEST--------------\n");
    //std::cout << request << std::endl;
    std::string path = request.getPath();

	std::string redirectTo;
	int statusCode;
	if ( ( statusCode = redirection(path, redirectTo, route) ) / 100 == 3){
		response.buildRedirectPage(request, statusCode, redirectTo);
		return ;
	}

	std::string fullPath = route->getFullPath(path);
    if (!utils::isFile(fullPath) && !autoindex(request, response, route)) // если не файл и автоиндекс не отработал
        fullPath = route->getDefaultFileName(path);

    if (response.getBody().empty())
    {
        response.setBody(utils::readFile(fullPath));
        response.setContentType(fullPath);
    }
    response.setStatusCode(200);
    response.setHeaderField("Host", request.getHost());
    response.setHeaderField("Content-Length", response.getBody().size() );
}

void HTTP::methodPOST(const Request& request, Response& response, Route* route){
    std::cout << request << std::endl;
    const std::string& path = route->getFullPath(request.getPath());
    const std::string& body = request.getBody();


    response.setStatusCode(201);
    response.setHeaderField("Content-Location", "/filename.xxx");
	response.setHeaderField("Content-Length", 0);
}

void HTTP::methodDELETE(const Request& request, Response& response, Route* route){
    const std::string& path = route->getFullPath(request.getPath());
    if (utils::isFile(path)) {
        if (remove(path.c_str()) == 0)
            response.buildDelPage(request);
        else
            throw httpEx<Forbidden>("Forbidden");
    }
    else
        throw httpEx<NotFound>("Not Found");
}

void HTTP::methodPUT(const Request & request, Response & response, Route *route) {
    const std::string& path = route->getFullPath(request.getPath());

    std::ifstream file(path);
    if (file.is_open()) {
        response.setStatusCode(204);
    }
    else {
        response.setStatusCode(201);
    }
    response.setHeaderField("Content-Length", 0);
}

void HTTP::methodHEAD(const Request& request, Response& response, Route* route) {
    std::string path = route->getFullPath(request.getPath());
    std::string body = utils::readFile(path);
    response.setStatusCode(200);
    response.setHeaderField("Host", request.getHost());
    response.setContentType(path);
    response.setHeaderField("Content-Length", body.size() );
}

void HTTP::methodCONNECT(const Request&, Response&, Route*){
    throw httpEx<MethodNotAllowed>("Not Allowed by subject");
}

void HTTP::methodOPTIONS(const Request&, Response&, Route*){
    throw httpEx<MethodNotAllowed>("Not Allowed by subject");
}

void HTTP::methodTRACE(const Request&, Response&, Route*){
    throw httpEx<MethodNotAllowed>("Not Allowed by subject");
}

void HTTP::methodPATCH(const Request&, Response&, Route*){
    throw httpEx<MethodNotAllowed>("Not Allowed by subject");
}

void HTTP::checkIfAllowed(const std::string& method, Route* route){
    std::vector<std::string> allowedMethods = route->getMethods();
    std::string headerAllow;

    for (std::vector<std::string>::iterator it = allowedMethods.begin(); it != allowedMethods.end(); ++it) {
        headerAllow += (it == allowedMethods.begin()) ? *it : ", " + *it;
    }

    if ( find(std::begin(allowedMethods), std::end(allowedMethods), method) == allowedMethods.end() ) {
        throw httpEx<MethodNotAllowed>(headerAllow); // put in structure string of allowed methods
    }
}

HTTP::MethodHttp 	HTTP::initMethods()
{
    HTTP::MethodHttp map;
    map["GET"] = &HTTP::methodGET;
    map["POST"] = &HTTP::methodPOST;
    map["DELETE"] = &HTTP::methodDELETE;
    map["PUT"] = &HTTP::methodPUT;
    map["HEAD"] = &HTTP::methodHEAD;
    map["CONNECT"] = &HTTP::methodCONNECT;
    map["OPTIONS"] = &HTTP::methodOPTIONS;
    map["TRACE"] = &HTTP::methodTRACE;
    map["PATCH"] = &HTTP::methodPATCH;
    return map;
}

HTTP::MethodHttp HTTP::_method = HTTP::initMethods();


void HTTP::startServer()
{
    std::vector<Server> servers = SettingsManager::getInstance()->getServers();
    HTTP serve;
    for (std::vector<Server>::const_iterator i = servers.begin(); i != servers.end(); i++) {
        char port[6];
        char ipstr[INET_ADDRSTRLEN];
        struct sockaddr_in sa;
        bzero(port, 6);
        sprintf(port, "%u", (*i).getPort());
        if (inet_pton(AF_INET, (*i).getHost().c_str(), &(sa.sin_addr)) < 1) {
            int status;
            struct addrinfo hints, *res, *p;
            memset(&hints, 0, sizeof hints);
            hints.ai_family = AF_INET;
            hints.ai_socktype = SOCK_STREAM;
            if ((status = getaddrinfo((*i).getHost().c_str(), port, &hints, &res)) != 0) {
                LOG_ERROR("Cannot get address from %s. Skipping...\n", (*i).getHost().c_str());
                continue;
            }
            for(p = res;p != NULL; p = p->ai_next) {
                void *addr;
                struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
                addr = &(ipv4->sin_addr);
                inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);

            }
            freeaddrinfo(p);
            freeaddrinfo(res);
        } else {
            strcpy(ipstr, (*i).getHost().c_str());
        }
        serve.listen(ipstr + std::string(":") + port);
    }
    serve.start();
}

bool HTTP::isValidMethod(const std::string &method)
{
    try {
        return _method.at(method) != nullptr;
    }
    catch (std::out_of_range &e) {return false;}
}

void HTTP::requestValidate(Request& request)
{
    if (!request.good()) {
        throw httpEx<BadRequest>("");
    }

    if (request.getPath()[0] != '/') {
        throw httpEx<BadRequest>("Invalid path");
    }

    const std::string& version = request.getVersion();
    size_t foundHTTP = version.find("HTTP/");

    if (foundHTTP == std::string::npos) {
        throw httpEx<BadRequest>("Invalid http version");
    }
    if (std::string(version, version.find_first_not_of("HTTP/")) != "1.1") {
        throw httpEx<HTTPVersionNotSupported>("HTTP version only HTTP/1.1");
    }
}

void HTTP::interpretResponseString(const std::string &responseString, Response &response)
{
	size_t foundNN = responseString.find("\n\n");
	size_t foundRN = responseString.find("\r\n\r\n");

	size_t found = std::min<size_t>(foundNN, foundRN);
	if (found != std::string::npos)
	{
		std::string line;
		size_t pos = responseString.find_first_not_of("\r\n", found);
		std::stringstream headers(std::string(responseString, 0, pos));
		while (true) {
			if (headers.eof()) break;
			std::getline(headers, line, '\n');
			std::pair<std::string, std::string> header = utils::breakPair(line, ':');
			if (header.first == "Status")
			{
				response.setStatusCode(atoi(header.second.c_str()));
				continue;
			}
			if (header.first.empty() || header.first == "Content-Length")
				continue;
			response.setHeaderField(header.first, header.second);
		}
		std::string body = "";
		if (pos != std::string::npos) {
			body = std::string(responseString, pos, responseString.size());
		}
		response.setHeaderField("Content-Length", body.size());
		response.setBody(body);
	}
}
