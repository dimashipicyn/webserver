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

const int64_t sessionTimeout = 2500000; // 2500 sec

typedef void (HTTP::*handlerFunc)(int, Session*);

struct Session
{
    void bind(handlerFunc f) {
        func = f;
    };

    std::string readBuf;
    std::string writeBuf;

    handlerFunc func;
    int32_t     fileDesc;
    int64_t     fileSize;
    int64_t     recvBytes;
    Request     request;
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

        Session listenSession;
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
    disableTimerEvent(id, 0);
    ::close(id);
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

        Session s;
        s.bind(&HTTP::defaultReadFunc);
        s.request.setHost(conn.getHost());
        s.request.setID(conn.getSock());

        newSessionByID(conn.getSock(), s);
        enableReadEvent(conn.getSock());
        enableTimerEvent(conn.getSock(), sessionTimeout); // TODO add config
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
        closeSessionByID(socket);
        return;
    }
    (this->*session->func)(socket, session);
}

void HTTP::asyncRead(int socket)
{
    LOG_DEBUG("Event reader call\n");

    Session *session = getSessionByID(socket);
    if (session == nullptr) {
        LOG_ERROR("Dont find session to socket: %d. Close connection.\n", socket);
        closeSessionByID(socket);
        return;
    }
    (this->*session->func)(socket, session);
}

void HTTP::asyncEvent(int socket, uint16_t flags)
{
    LOG_DEBUG("Event handler call\n");
    if (flags & EventPool::M_EOF || flags & EventPool::M_ERROR) {
        LOG_ERROR("Event error or eof, socket: %d\n", socket);
        closeSessionByID(socket);
    }
    if (flags & EventPool::M_TIMER) {
        LOG_ERROR("Event timeout, socket: %d\n", socket);
        closeSessionByID(socket);
    }
}

void HTTP::defaultReadFunc(int socket, Session *session)
{
    LOG_DEBUG("defaultReadFunc call\n");
    const int64_t bufSize = (1 << 16);
    char buf[bufSize];

    int64_t readBytes = ::read(socket, buf, bufSize - 1);
    if (readBytes < 0) {
        closeSessionByID(socket);
        LOG_ERROR("Dont read to socket: %d. Close connection.\n", socket);
        return;
    }

    buf[readBytes] = '\0';

    std::string& rbuf = session->readBuf;

    rbuf.append(buf);
    LOG_DEBUG("rbuf: %s", rbuf.c_str());

    size_t foundNN = rbuf.find("\n\n");
    size_t foundRN = rbuf.find("\r\n\r\n");

    if (foundNN != std::string::npos) {
        size_t pos = rbuf.find_first_not_of("\r\n", foundNN);
        session->request.reset();
        session->request.parse(std::string(rbuf, 0, pos));
        rbuf.erase(0, pos);
    }
    if (foundRN != std::string::npos) {
        size_t pos = rbuf.find_first_not_of("\r\n", foundRN);
        session->request.reset();
        session->request.parse(std::string(rbuf, 0, pos));
        rbuf.erase(0, pos);
    }

    if (foundNN != std::string::npos || foundRN != std::string::npos)
    {
        session->bind(&HTTP::defaultWriteFunc);
        // включаем write
        enableWriteEvent(socket);
        disableReadEvent(socket);
    }
}

void HTTP::defaultWriteFunc(int socket, Session *session)
{
    LOG_DEBUG("defaultWriteFunc call\n");
    session->bind(&HTTP::doneFunc);

    Request& request = session->request;

    Response response;
    handler(request, response);

    std::string& wbuf = session->writeBuf;

    wbuf.append(response.getContent());
}

void HTTP::doneFunc(int socket, Session *session)
{
    LOG_DEBUG("doneFunc call\n");
    std::string& wbuf = session->writeBuf;

    if (wbuf.empty()) {
        disableWriteEvent(socket);
        enableReadEvent(socket);
        session->bind(&HTTP::defaultReadFunc);
    }
    else {
        int64_t writeBytes = ::write(socket, wbuf.c_str(), wbuf.size());

        if (writeBytes < 0) {
            closeSessionByID(socket);
            LOG_ERROR("Dont write to socket: %d. Close connection.\n", socket);
            return;
        }

        wbuf.erase(0, writeBytes);
    }
}

void HTTP::sendFileFunc(int socket, Session* session)
{
    LOG_DEBUG("sendFileFunc call\n");
    const int32_t bufSize = (1 << 16);
    char buf[bufSize] = {};

    int32_t fd = session->fileDesc;
    int32_t readBytes = ::read(fd, buf, bufSize - 1);

    if (readBytes == 0) {
        close(fd);
        session->bind(&HTTP::doneFunc);
    }

    if (readBytes < 0) {
        closeSessionByID(socket);
        LOG_ERROR("Dont read to file: %d. Close connection.\n", socket);
        return;
    }

    buf[readBytes] = '\0';

    std::string& wbuf = session->writeBuf;
    wbuf.append(buf);

    int32_t writeBytes = ::write(socket, wbuf.c_str(), wbuf.size());

    if (writeBytes < 0) {
        close(fd);
        closeSessionByID(socket);
        LOG_ERROR("Dont write to socket: %d. Close connection.\n", socket);
        return;
    }

    wbuf.erase(0, writeBytes);
}

void HTTP::recvFileFunc(int socket, Session* session)
{
    LOG_DEBUG("recvFileFunc call\n");
    const int32_t bufSize = (1 << 16);
    char buf[bufSize] = {};


    int32_t readBytes = ::read(socket, buf, bufSize - 1);

    if (readBytes < 0) {
        closeSessionByID(socket);
        LOG_ERROR("Dont read to file: %d. Close connection.\n", socket);
        return;
    }

    buf[readBytes] = '\0';


    std::string& wbuf = session->writeBuf;
    wbuf.append(buf);

    int32_t fd = session->fileDesc;

    int64_t bytes = 0;
    if (session->fileSize != -1) {
        bytes = std::min<int64_t>(wbuf.size(), session->fileSize - session->recvBytes);
    }
    else {
        bytes = wbuf.size();
    }


    int32_t writeBytes = ::write(fd, wbuf.c_str(), bytes);

    if (writeBytes < 0) {
        closeSessionByID(socket);
        LOG_ERROR("Dont write to socket: %d. Close connection.\n", socket);
        return;
    }

    session->recvBytes += writeBytes;
    if (session->recvBytes == session->fileSize) {
        session->bind(&HTTP::cgiCaller);
        disableReadEvent(socket);
        enableWriteEvent(socket);
    }

    if (session->fileSize == -1 && wbuf.size()) {
        session->bind(&HTTP::cgiCaller);
        disableReadEvent(socket);
        enableWriteEvent(socket);
    }

    wbuf.erase(0, writeBytes);
}

void HTTP::recvChunkedReadFromSocket(int socket, Session *session)
{
    const int32_t bufSize = (1 << 16);
    char buf[bufSize] = {};


    int32_t readBytes = ::read(socket, buf, bufSize - 1);

    if (readBytes < 0) {
        closeSessionByID(socket);
        LOG_ERROR("Dont read to file: %d. Close connection.\n", socket);
        return;
    }

    buf[readBytes] = '\0';

    std::string& rbuf = session->readBuf;

    rbuf.append(buf);

    size_t found = rbuf.find("\n");
    if (found != std::string::npos) {
        session->fileSize = utils::to_number<int64_t>(std::string(rbuf, 0, found + 1));
        session->recvBytes = 0;
        session->bind(&HTTP::recvChunkedWriteToFile);
        rbuf.erase(0, found + 1);
    }
}

void HTTP::recvChunkedWriteToFile(int socket, Session* session)
{
    const int32_t bufSize = (1 << 16);
    char buf[bufSize] = {};


    int32_t readBytes = ::read(socket, buf, bufSize - 1);

    if (readBytes < 0) {
        closeSessionByID(socket);
        LOG_ERROR("Dont read to file: %d. Close connection.\n", socket);
        return;
    }

    buf[readBytes] = '\0';

    std::string& rbuf = session->readBuf;

    rbuf.append(buf);

    size_t size = session->fileSize;
    int32_t fd = session->fileDesc;

    if (size == 0) {
        rbuf.erase(0, rbuf.find_first_not_of("\r\n"));
        disableReadEvent(socket);
        enableWriteEvent(socket);
        session->bind(&HTTP::doneFunc);
    }

    if (rbuf.size() > size) {



        while (size > 0) {
            size_t writeBytes = ::write(fd, rbuf.c_str(), size);

            if (writeBytes < 0) {
                closeSessionByID(socket);
                LOG_ERROR("Dont write to socket: %d. Close connection.\n", socket);
                return;
            }

            rbuf.erase(0, writeBytes);
            size -= writeBytes;
        }

        rbuf.erase(0, rbuf.find_first_not_of("\r\n"));
        session->bind(&HTTP::recvChunkedReadFromSocket);
    }
}

void HTTP::recvChunkedWriteToSocket(int socket, Session* session)
{

}

void HTTP::recvChunkedReadFromFile(int socket, Session* session)
{
    const int32_t bufSize = (1 << 16);
    char buf[bufSize] = {};

    int32_t fd = session->fileDesc;
    int32_t readBytes = ::read(fd, buf, bufSize - 1);

    if (readBytes < 0) {
        close(fd);
        closeSessionByID(socket);
        LOG_ERROR("Dont read to file: %d. Close connection.\n", socket);
        return;
    }

    buf[readBytes] = '\0';

    std::string rbuf(utils::to_string<int32_t>(readBytes));
    rbuf.append("\r\n");
    rbuf.append(buf);
    rbuf.append("\r\n");

    while (!rbuf.empty()) {
        size_t writeBytes = ::write(socket, rbuf.c_str(), rbuf.size());

        if (writeBytes < 0) {
            close(fd);
            closeSessionByID(socket);
            LOG_ERROR("Dont write to socket: %d. Close connection.\n", socket);
            return;
        }

        rbuf.erase(0, writeBytes);
    }

    if (readBytes == 0) {
        session->bind(&HTTP::defaultReadFunc);
        enableReadEvent(socket);
        disableWriteEvent(socket);
        close(fd);
    }
}

void HTTP::cgiCaller(int socket, Session* session)
{
    LOG_DEBUG("CgiCaller call\n");
    Request& request = session->request;

    Response response;
    cgi(request, response, nullptr);

    session->writeBuf.append(response.getContent());
    session->bind(&HTTP::doneFunc);
}

///////////////////////////////////////////////////////////////////////////
///////////////////////// HTTP LOGIC //////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

bool HTTP::cgi(const Request &request, Response& response, Route* route) {
    const std::string& path = request.getPath();
    bool isCGI = route != nullptr && utils::getExtension(path) == route->getCgi();
    if (isCGI) {
        if (!utils::isFile(route->getFullPath(path)))
            throw httpEx<NotFound>("CGI script not found");
        std::string body = Cgi(request, *route).runCGI();
        //        response.setHeaderField("Content-Type", request.getHeaders().at("Content-Type"));
        response.setHeaderField("Content-Length", body.size());
        response.setStatusCode(200);
        response.setBody(body);
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

// здесь происходит обработка запроса
void HTTP::handler(Request& request, Response& response) {
	SettingsManager *settingsManager = SettingsManager::getInstance();
	Server *server = settingsManager->findServer(request.getHost());
	Route *route = (server == nullptr ? nullptr : server->findRouteByPath(request.getPath()));
	try {
        LOG_DEBUG("Http handler call\n");
        LOG_DEBUG("--------------PRINT REQUEST--------------\n");
        std::cout << request << std::endl;


        /*
        response.setStatusCode(200);
        //recvFileChunked(request, response, "./test_chunked");
        sendFileChunked(request, response, "./test.sh");
        return;

        if (request.getMethod() == "GET") {
            sendFile(request, response, "./index.html");
            return;
        }
        if (request.getMethod() == "POST") {
            recvFile(request, response, "./my_file");
            return;
        }
        */


        // Сравниваем расширение запрошенного ресурса с cgi расширением для этого локейшена. Если бьется, запуск скрипта
        checkIfAllowed(request, route);

        if (route == nullptr) {
            throw httpEx<NotFound>("Not Found");
        }
        std::string method = request.getMethod();
        if (method.empty() ) throw httpEx<BadRequest>("Invalid Request");
        try {
            if (!cgi(request, response, route))
                (this->*_method.at(method))(request, response, route);
        }
        catch (const std::out_of_range& e) {
            throw httpEx<BadRequest>("Invalid Request");
        }
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
    std::string path = request.getPath();

	std::string redirectTo;
	int statusCode;
	if ( ( statusCode = redirection(path, redirectTo, route) ) / 100 == 3){
		response.setStatusCode(statusCode);
		response.setHeaderField("Location", redirectTo);
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
    response.writeFile(path, body);
    response.setStatusCode(201);
    //response.setHeaderField("Content-Location", "/filename.xxx");
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
    response.writeContent(path, request);
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

void HTTP::checkIfAllowed(const Request& request, Route *route){
    std::vector<std::string> allowedMethods = route->getMethods();
    std::string headerAllow;
    for (std::vector<std::string>::iterator it = allowedMethods.begin(); it != allowedMethods.end(); ++it) {
        headerAllow += (it == allowedMethods.begin()) ? *it : ", " + *it;
    }
    if ( find(std::begin(allowedMethods), std::end(allowedMethods), request.getMethod()) == allowedMethods.end() ) {
        throw httpEx<MethodNotAllowed>(headerAllow); // put in structure string of allowed methods
        //response.setHeaderField("Allow", "GET"); // need to put all allowed methods here from config set
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


void HTTP::sendFile(Request& request, Response& response, const std::string& path)
{
    int fd = ::open(path.c_str(), O_RDONLY);

    if (fd == -1) {
        throw httpEx<InternalServerError>("Cannot open file: " + path);
    }

    struct stat info;

    if (::fstat(fd, &info) == -1) {
        throw httpEx<InternalServerError>("Cannot stat file info: " + path);
    }

    int16_t sizeFile = info.st_size;

    response.setStatusCode(200);
    response.setHeaderField("Content-Length", sizeFile);
    response.setContent(response.getHeader());

    Session* session = getSessionByID(request.getID());

    session->fileDesc = fd;
    session->bind(&HTTP::sendFileFunc);
}

void HTTP::recvFile(Request& request, Response&, const std::string& path)
{
    int fd = ::open(path.c_str(), O_WRONLY|O_CREAT, 0664);

    if (fd == -1) {
        throw httpEx<InternalServerError>("Cannot open file: " + path);
    }

    Session* session = getSessionByID(request.getID());

    session->fileDesc = fd;
    session->recvBytes = 0;
    if (request.hasHeader("Content-Length")) {
        session->fileSize = utils::to_number<int64_t>(request.getHeaderValue("Content-Length"));
    }
    else {
        session->fileSize = -1;
    }

    session->bind(&HTTP::recvFileFunc);
    enableReadEvent(request.getID());
    disableWriteEvent(request.getID());
}

void HTTP::recvFileChunked(Request& request, Response& response, const std::string& path)
{
    int fd = ::open(path.c_str(), O_WRONLY|O_CREAT, 0664);

    if (fd == -1) {
        throw httpEx<InternalServerError>("Cannot open file: " + path);
    }

    Session* session = getSessionByID(request.getID());

    session->fileDesc = fd;
    session->recvBytes = 0;
    session->fileSize = 0;

    session->bind(&HTTP::recvChunkedReadFromSocket);
    enableReadEvent(request.getID());
    disableWriteEvent(request.getID());
}

void HTTP::sendFileChunked(Request& request, Response& response, const std::string& path)
{
    int fd = ::open(path.c_str(), O_RDONLY);

    if (fd == -1) {
        throw httpEx<InternalServerError>("Cannot open file: " + path);
    }

    Session* session = getSessionByID(request.getID());

    session->fileDesc = fd;
    session->recvBytes = 0;
    session->fileSize = 0;

    session->bind(&HTTP::recvChunkedReadFromFile);
}

bool HTTP::isValidMethod(const std::string &method)
{
    try {
        return _method.at(method) != nullptr;
    }
    catch (std::out_of_range &e) {return false;}
}
