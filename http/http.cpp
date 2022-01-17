#include <iostream>
#include <netdb.h>
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

struct Session
{
    Session()
        : host()
        , writeBuffer()
        , readBuffer()
    {

    }
    Session(const std::string& host)
        : host(host)
        , writeBuffer()
        , readBuffer()
    {

    };
    Session(Session& session)
        : host(session.host)
        , writeBuffer(session.writeBuffer)
        , readBuffer(session.readBuffer)
    {
    };
    Session& operator=(Session& session) {
        if (&session == this) {
            return *this;
        }
        host = session.host;
        writeBuffer = session.writeBuffer;
        readBuffer = session.readBuffer;
        return *this;
    };
    ~Session() {

    };

    std::string         host;
    std::string         writeBuffer;
    std::string         readBuffer;
};

HTTP::HTTP()
    : sessionMap_()
{

}

HTTP::~HTTP()
{

}

void HTTP::start() {
    EventPool::start();
}

void HTTP::listen(const std::string& host)
{
    try {
        TcpSocket conn(host);
        conn.listen();
        conn.makeNonBlock();

        Session listenSession(conn.getHost());
        newListenerEvent(conn);
        newSessionByID(conn.getSock(), listenSession);
    }  catch (std::exception& e) {
        LOG_ERROR("HTTP: %s\n", e.what());
    }
}

Session* HTTP::getSessionByID(int id)
{
    if (sessionMap_.find(id) != sessionMap_.end()) {
        return &sessionMap_[id];
    }
    return nullptr;
}

void HTTP::closeSessionByID(int id) {
    sessionMap_.erase(id);
}

void HTTP::newSessionByID(int id, Session& session)
{
    sessionMap_[id] = session;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////// ASYNC LOGIC /////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void HTTP::asyncAccept(TcpSocket& socket)
{
    LOG_DEBUG("Event accepter call\n");

    try
    {
        Session *session = getSessionByID(socket.getSock());
        if (session == nullptr) {
            LOG_ERROR("Dont find session to socket: %d. Close.\n", socket.getSock());
            close(socket.getSock());
            return;
        }

        TcpSocket conn = socket.accept();

        LOG_DEBUG("New connect fd: %d\n", conn.getSock());

        Session s(conn.getHost());
        newSessionByID(conn.getSock(), s);
        enableReadEvent(conn.getSock());
    }
    catch (std::exception& e)
    {
        LOG_ERROR("HTTP::asyncAccept error: %s\n", e.what());
    }
};

void HTTP::asyncWrite(int socket)
{
    LOG_DEBUG("Event writer call\n");

    Session *session = getSessionByID(socket);
    if (session == nullptr) {
        LOG_ERROR("Dont find session to socket: %d. Close connection.\n", socket);
        close(socket);
        return;
    }
//===============
    Response response;
    Request request;

    request.parse(session->readBuffer.c_str());
    request.setHost(session->host);

    session->readBuffer.clear();
    handler(request, response);

    session->writeBuffer.append(response.getContent());
    std::string& writeBuffer = session->writeBuffer;
    int64_t writeBytes = ::write(socket, writeBuffer.c_str(), writeBuffer.size());
    session->writeBuffer.erase(0, writeBytes);

    // выключаем write
    disableWriteEvent(socket);
}

void HTTP::asyncRead(int socket)
{
    LOG_DEBUG("Event reader call\n");

    Session *session = getSessionByID(socket);
    if (session == nullptr) {
        LOG_ERROR("Dont find session to socket: %d. Close connection.\n", socket);
        close(socket);
        return;
    }

    std::string buffer;
    int64_t bufferSize = (1 << 16);

    buffer.resize(bufferSize);

    int64_t readBytes = ::read(socket, &buffer[0], bufferSize - 1);
    if (readBytes < 0) {
        close(socket);
        LOG_ERROR("Dont read to socket: %d. Close connection.\n", socket);
    }

    session->readBuffer.append(buffer.c_str());

    // включаем write
    enableWriteEvent(socket);
}

void HTTP::asyncEvent(int socket, uint16_t flags)
{
    LOG_DEBUG("Event handler call\n");
    if (flags & EventPool::M_EOF || flags & EventPool::M_ERROR) {
        LOG_ERROR("Event error or eof, socket: %d\n", socket);
        close(socket);
    }
}

///////////////////////////////////////////////////////////////////////////
///////////////////////// HTTP LOGIC //////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

void HTTP::cgi(const Request &request, Response& response, Route* route) {
    const std::string& path = request.getPath();

    if (route != nullptr && utils::getExtension(path) == route->getCgi()) {
        response.setContent(Cgi(request, *route).runCGI());
    }
}

void HTTP::autoindex(const Request &request, Response &response, Route *route) {
    const std::string& path = request.getPath();

    if (route != nullptr && route->isAutoindex() && utils::getExtension(path).empty()) {
        std::stringstream header;
        std::string html;
        try
        {
            html = route->getDefaultPage(path);
        } catch (Route::DefaultFileNotFoundException &e) {
            LOG_DEBUG("Default file at %s not found. Proceed autoindexing.\n", path.c_str());
			html = Autoindex(*route).generatePage(path);
        }
        header << "HTTP/1.1 200 OK\n"
               << "Content-Length: " << html.size() << "\n"
               << "Content-Type: text/html\n\n";
        response.setContent(header.str() + html);
    }
}

// здесь происходит обработка запроса
void HTTP::handler(Request& request, Response& response) {
	try {
		LOG_DEBUG("Http handler call\n");
		LOG_DEBUG("--------------PRINT REQUEST--------------\n");
		std::cout << request << std::endl;

		// Сравниваем расширение запрошенного ресурса с cgi расширением для этого локейшена. Если бьется, запуск скрипта
		SettingsManager *settingsManager = SettingsManager::getInstance();
		Server *server = settingsManager->findServer(request.getHost());
		Route *route = server == nullptr ? nullptr : server->findRouteByPath(
				request.getPath());
		if (route == nullptr) {
			throw httpEx<NotFound>("Not Found");
		}
		std::string method = request.getMethod();
		if (_method.count(method))
			(this->*_method.at(method))(
					request, response, route); // updating idea: here we can use try catch (if bad method) catch badrequest
			//also make pointers for all HTTP methods and in that methods implementation compare method with config allowed
		else if (_allMethods.count(method)) {
			methodNotAllowed(request, response);
		}
		else {
			throw httpEx<BadRequest>("Bad Request");
		}
	}
	catch (httpEx<BadRequest> &e) {
		LOG_INFO("BadRequest: %s\n", e.what());
	}
	catch (httpEx<Unauthorized> &e) {
		LOG_INFO("Unauthorized: %s\n", e.what());
	}
	catch (httpEx<PaymentRequired> &e) {
		LOG_INFO("PaymentRequired: %s\n", e.what());
	}
	catch (httpEx<Forbidden> &e) {
		LOG_INFO("Forbidden: %s\n", e.what());
	}
	catch (httpEx<NotFound> &e) {
		LOG_INFO("NotFound: %s\n", e.what());
	}
	catch (httpEx<MethodNotAllowed> &e) {
		LOG_INFO("MethodNotAllowed: %s\n", e.what());
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
		std::cout << request << std::endl;

		cgi(request, response, route);
		autoindex(request, response, route);
		if (!response.getContent().empty()) return; // Костыльно, но времени не хватит для более глубокой интеграции

		std::string path = route->getFullPath(request.getPath());
		std::ifstream sourceFile;
		std::string content;
		if ( !sourceFile.good() ){
			//path = response.getErrorPath(request); set errorfile from config here
			std::string errorPage = response.getErrorPage(404);
			response.setStatusCode(404);
			response.setHeaderField("Content-Type: ", "text/html");
			response.setHeaderField("Content-Length", errorPage.size() );
			response.setContent(response.getHeader() + errorPage);
			} else {
			std::string content((std::istreambuf_iterator<char>(sourceFile)),
								std::istreambuf_iterator<char>());
			sourceFile.close();
			response.setStatusCode(200);
			response.setContentType(path);
			response.setHeaderField("Content-Length", content.size() );
			response.setContent(response.getHeader() + content);
		}
	}

	void HTTP::methodPOST(const Request& request, Response& response, Route* route){}
	void HTTP::methodDELETE(const Request& request, Response& response, Route* route){}
	void HTTP::methodNotAllowed(const Request& request, Response& response){
		response.setStatusCode(405);
		std::string strAllowMethods;
		for (std::map<std::string, void (HTTP::*)(const Request &, Response&, Route*)>::iterator it = _method.begin();
			 it != _method.end(); ++it){
			if (it == _method.begin()) strAllowMethods += it->first;
			else strAllowMethods = strAllowMethods + ", " + it->first;
		}
		response.setHeaderField("Allow", strAllowMethods);
	}

/*
	void HTTP::BadRequest(Response& response){
		response.setStatusCode(400);
		std::stringstream ss;
		std::string s("Bad Request\n");
		response.setHeaderField("Content-Length", s.size());
		response.setContentType("text/html");
		response.setContent(response.getHeader() + s);
	}*/
//=============================================================//

//==============================Moved from Response class=====================

    HTTP::MethodHttp 	HTTP::initMethods()
	{
		std::map<std::string, void (HTTP::*)(const Request &, Response&, Route*)> map;
		map["GET"] = &HTTP::methodGET;
		map["POST"] = &HTTP::methodPOST;
		map["DELETE"] = &HTTP::methodDELETE;
		return map;
	}

	HTTP::MethodHttp HTTP::_method
			= HTTP::initMethods();

	std::set<std::string> HTTP::initAllMethods(){
		std::set<std::string> allMethods;
		allMethods.insert("GET");
		allMethods.insert("HEAD");
		allMethods.insert("POST");
		allMethods.insert("PUT");
		allMethods.insert("DELETE");
		allMethods.insert("CONNECT");
		allMethods.insert("OPTIONS");
		allMethods.insert("TRACE");
		allMethods.insert("PATCH");
		return allMethods;
	}

	std::set<std::string> HTTP::_allMethods = initAllMethods();
//=========================================================================


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
