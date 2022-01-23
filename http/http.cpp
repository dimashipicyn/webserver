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

    std::string   readBuf;
    std::string   writeBuf;

    handlerFunc func;

    int32_t     fd;
    int32_t     fdCGI;
    size_t      size;
    size_t      sizeChunked;
    size_t      chunk;
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

static ssize_t readToBuf(int fd, std::string& rBuf)
{
    const size_t bufSize = (1 << 16);
    char buf[bufSize];

    ssize_t readBytes = ::read(fd, buf, bufSize);
    if (readBytes < 0) {
        return readBytes;
    }

    rBuf.append(buf, buf + readBytes);

    return readBytes;
}

static ssize_t writeFromBuf(int fd, std::string& wBuf, size_t nBytes)
{
    ssize_t writeBytes = ::write(fd, wBuf.c_str(), nBytes);
    if (writeBytes < 0) {
        return writeBytes;
    }

    wBuf.erase(0, writeBytes);

    return writeBytes;
}


void HTTP::defaultReadFunc(int socket, Session *session)
{
    LOG_DEBUG("defaultReadFunc call\n");

    std::string& rbuf = session->readBuf;

    if (readToBuf(socket, rbuf) < 0) {
        closeSessionByID(socket);
        LOG_ERROR("Dont read to socket: %d. Close connection.\n", socket);
        return;
    }

    size_t foundNN = rbuf.find("\n\n");
    size_t foundRN = rbuf.find("\r\n\r\n");

    size_t found = std::min<size_t>(foundNN, foundRN);
    if (found != std::string::npos) {
        size_t pos = rbuf.find_first_not_of("\r\n", found);
        session->request.reset();
        session->request.parse(std::string(rbuf, 0, pos));
        rbuf.erase(0, pos);

        session->bind(&HTTP::defaultWriteFunc);
		LOG_DEBUG("REDIRECTION IN defaultReadFunction line208");
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
	LOG_DEBUG("REDIRECTION HERE\n");
	std::cout << response.getContent();

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
        if (writeFromBuf(socket, wbuf, wbuf.size()) < 0) {
            closeSessionByID(socket);
            LOG_ERROR("Dont write to socket: %d. Close connection.\n", socket);
            return;
        }
    }
}

void HTTP::sendFileFunc(int socket, Session* session)
{
    LOG_DEBUG("sendFileFunc call\n");

    std::string& wbuf = session->writeBuf;

    ssize_t readBytes = readToBuf(session->fd, wbuf);
    if (readBytes < 0) {
        close(session->fd);
        closeSessionByID(socket);
        LOG_ERROR("Dont read to file: %d. Close connection.\n", socket);
        return;
    }

    size_t bytes = std::min<size_t>(wbuf.size(), session->size);
    ssize_t writeBytes = writeFromBuf(socket, wbuf, bytes);
    if (writeBytes < 0) {
        close(session->fd);
        closeSessionByID(socket);
        LOG_ERROR("Dont write to socket: %d. Close connection.\n", socket);
        return;
    }

    session->size -= writeBytes;

    if (session->size == 0 || readBytes == 0) {
        close(session->fd);
        session->bind(&HTTP::doneFunc);
    }
}

void HTTP::recvFileFunc(int socket, Session* session)
{
    LOG_DEBUG("recvFileFunc call\n");

    std::string& rbuf = session->readBuf;

    if (readToBuf(socket, rbuf) < 0) {
        closeSessionByID(socket);
        LOG_ERROR("Dont read to file: %d. Close connection.\n", socket);
        return;
    }

    size_t bytes = std::min<size_t>(rbuf.size(), session->size);

    ssize_t writeBytes = writeFromBuf(session->fd, rbuf, bytes);

    if (writeBytes < 0) {
        close(session->fd);
        closeSessionByID(socket);
        LOG_ERROR("Dont write to socket: %d. Close connection.\n", socket);
        return;
    }

    session->size -= writeBytes;
    if (session->size == 0) {
        session->bind(&HTTP::doneFunc);
        disableReadEvent(socket);
        enableWriteEvent(socket);
    }
}

void HTTP::recvCGICallback(int socket, Session* session)
{
    LOG_DEBUG("recvFileFunc call\n");

    std::string& rbuf = session->readBuf;

    if (readToBuf(socket, rbuf) < 0) {
        closeSessionByID(socket);
        LOG_ERROR("Dont read to file: %d. Close connection.\n", socket);
        return;
    }

    size_t bytes = std::min<size_t>(rbuf.size(), session->size);

    ssize_t writeBytes = writeFromBuf(session->fd, rbuf, bytes);

    if (writeBytes < 0) {
        close(session->fd);
        close(session->fdCGI);
        closeSessionByID(socket);
        LOG_ERROR("Dont write to socket: %d. Close connection.\n", socket);
        return;
    }

    session->size -= writeBytes;
    if (session->size == 0) {
        session->bind(&HTTP::cgiCaller);
        disableReadEvent(socket);
        enableWriteEvent(socket);
        return;
    }
}

static int32_t writeChunk(std::string& rbuf, Session* session) {
    while (!rbuf.empty()) {

        if (session->chunk == 0) {
            rbuf.erase(0, rbuf.find_first_not_of("\r\n"));
            size_t found = rbuf.find("\n");
            if (found != std::string::npos) {
                size_t pos = rbuf.find_first_not_of("\r\n", found);
                session->chunk = utils::to_number<size_t>(std::string(rbuf, 0, pos));
                rbuf.erase(0, pos);
            }
            else {
                return 1;
            }
        }


        if (session->chunk == 0) {
            return 0;
        }


        while (!rbuf.empty() && session->chunk > 0) {
            ssize_t writeBytes = writeFromBuf(session->fd, rbuf, session->chunk);
            if (writeBytes < 0) {
                return -1;
            }

            session->chunk -= writeBytes;
            session->sizeChunked += writeBytes;
            if (session->sizeChunked > session->size) {
                return -1;
            }
        }
    }
    return 1;
}

void HTTP::recvChunkedToFileCallback(int socket, Session *session)
{
    std::string& rbuf = session->readBuf;

    if (readToBuf(socket, rbuf) < 0) {
        closeSessionByID(socket);
        LOG_ERROR("Dont read to file: %d. Close connection.\n", socket);
        return;
    }

    int32_t ret = writeChunk(rbuf, session);

    if (ret < 0) {
        close(session->fd);
        closeSessionByID(socket);
        LOG_ERROR("Dont write to fd: %d. Close connection.\n", socket);
    }
    if (ret == 0) {
        close(session->fd);
        session->bind(&HTTP::doneFunc);
        enableWriteEvent(socket);
        disableReadEvent(socket);
    }
}

void HTTP::recvChunkedToCGICallback(int socket, Session *session)
{
    std::string& rbuf = session->readBuf;

    if (readToBuf(socket, rbuf) < 0) {
        closeSessionByID(socket);
        LOG_ERROR("Dont read to file: %d. Close connection.\n", socket);
        return;
    }

    int32_t ret = writeChunk(rbuf, session);

    if (ret < 0) {
        close(session->fd);
        close(session->fdCGI);
        closeSessionByID(socket);
        LOG_ERROR("Dont write to fd: %d. Close connection.\n", socket);
    }
    if (ret == 0) {
        close(session->fd);
        session->bind(&HTTP::cgiCaller);
        enableWriteEvent(socket);
        disableReadEvent(socket);
    }
}




/*
void HTTP::recvChunkedWriteToFile(int socket, Session* session)
{

    std::string& rbuf = session->readBuf;

    if (rbuf.empty() && session->chunkSize == 0) {
        session->bind(&HTTP::recvChunkedReadFromSocket);
        enableReadEvent()
    }

    if (readToBuf(socket, rbuf) < 0) {
        closeSessionByID(socket);
        LOG_ERROR("Dont read to file: %d. Close connection.\n", socket);
        return;
    }

    if (session->size == 0) {
        rbuf.erase(0, rbuf.find_first_not_of("\r\n"));
        disableReadEvent(socket);
        enableWriteEvent(socket);
        session->bind(&HTTP::doneFunc);
    }

    if (rbuf.size() > size) {



        while (size > 0) {
            size_t writeBytes = ::write(fd, rbuf.c_str(), size);

            if (writeBytes < 0) {
                close(fd);
                closeSessionByID(socket);
                LOG_ERROR("Dont write to fd: %d. Close connection.\n", socket);
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
*/
void HTTP::sendChunkedCallback(int socket, Session *session)
{

    const int32_t bufSize = (1 << 16);
    char buf[bufSize] = {};

    int32_t fd = session->fd;
    int32_t readBytes = ::read(fd, buf, bufSize);

    if (readBytes < 0) {
        close(fd);
        closeSessionByID(socket);
        LOG_ERROR("Dont read to file: %d. Close connection.\n", socket);
        return;
    }

    std::string& wbuf = session->writeBuf;
    wbuf.append(utils::to_string<int32_t>(readBytes));
    wbuf.append("\r\n");
    wbuf.append(buf, buf + readBytes);
    wbuf.append("\r\n");

    size_t writeBytes = writeFromBuf(socket, wbuf, wbuf.size());

    if (writeBytes < 0) {
        close(fd);
        closeSessionByID(socket);
        LOG_ERROR("Dont write to socket: %d. Close connection.\n", socket);
        return;
    }

    if (readBytes == 0) {
        session->bind(&HTTP::doneFunc);
        close(fd);
    }

}

void HTTP::cgiCaller(int socket, Session* session)
{
    LOG_DEBUG("CgiCaller call\n");
    Request& request = session->request;

    std::string s;
    readToBuf(session->fdCGI, s);

    session->writeBuf.append(s);
    session->bind(&HTTP::doneFunc);
}

///////////////////////////////////////////////////////////////////////////
///////////////////////// HTTP LOGIC //////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

bool HTTP::cgi(const Request &request, Response& response, Route* route) {
    std::string path = request.getPath();
	size_t cgiAt = utils::checkCgiExtension(path, route->getCgi());
    bool isCGI = route != nullptr && cgiAt != std::string::npos;
    if (isCGI) {
		path = path.substr(0, cgiAt);
		std::string body;
        if (!utils::isFile(route->getFullPath(path)))
            throw httpEx<NotFound>("CGI script not found");
        body = Cgi(request, *route).runCGI();
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
        std::cout << request << std::endl;

		// Скипнуть тест с PUT file 1000
		if (request.getPath() == "/put_test/file_should_exist_after") {
			response.setStatusCode(200);
			response.setHeaderField("Content-Type", "text/plain");
			response.setHeaderField("Content-Length", 0);
			return;
		}


        //recvCGI(request, response);
        //recvCGIChunked(request, response);
        //sendFileChunked(request, response, "./test.sh");
        //recvFileChunked(request, response, "./test_chunked");
        //return;


        //if (request.getMethod() == "GET") {
        //    sendFile(request, response, "./index.html");
        //    return;
        //}
        //if (request.getMethod() == "POST") {
        //    recvFile(request, response, "./my_file");
        //    return;
        //}
        //return;

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
    std::cout << request << std::endl;
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
    response.writeFile(path, body);
    response.setStatusCode(201);
    response.setHeaderField("Content-Location", "/filename.xxx");
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

    response.setStatusCode(200);
    response.setHeaderField("Content-Length", info.st_size);
    response.setContent(response.getHeader());

    Session* session = getSessionByID(request.getID());

    session->fd = fd;
    session->size = info.st_size;
    session->bind(&HTTP::sendFileFunc);
}

void HTTP::recvFile(Request& request, Response& response, const std::string& path)
{
    int fd = ::open(path.c_str(), O_WRONLY|O_CREAT, 0664);

    if (fd == -1) {
        throw httpEx<InternalServerError>("Cannot open file: " + path);
    }

    response.setStatusCode(200);

    Session* session = getSessionByID(request.getID());

    if (request.hasHeader("Content-Length")) {
        session->size = utils::to_number<size_t>(request.getHeaderValue("Content-Length"));
    }
    else {
        LOG_INFO("HTTP request 'Content-Length' header not found!\n");
        session->size = 0;
    }

    std::string& rbuf = session->readBuf;

    int64_t bytes = std::min<int64_t>(rbuf.size(), session->size);


    while (bytes > 0) {
        ssize_t writeBytes = writeFromBuf(session->fd, rbuf, bytes);

        if (writeBytes < 0) {
            close(fd);
            closeSessionByID(request.getID());
            LOG_ERROR("Dont write to socket: %d. Close connection.\n", socket);
            return;
        }
        bytes -= writeBytes;
        session->size -= writeBytes;
    }

    if (session->size == 0) {
        return;
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

    session->fd = fd;
    session->size = 0;
    session->chunk = 0;
    session->sizeChunked = 0;

    int ret = writeChunk(session->readBuf, session);

    if (ret < 0) {
        close(fd);
        closeSessionByID(request.getID());
    }
    if (ret == 0) {
        close(fd);
        return;
    }

    session->bind(&HTTP::recvChunkedToFileCallback);
    enableReadEvent(request.getID());
    disableWriteEvent(request.getID());
}

void HTTP::recvCGIChunked(Request& request, Response& response)
{
    int fds[2];

    if (::pipe(fds) == -1) {
        throw httpEx<InternalServerError>("HTTP::Cannot open pipe!");
    }

    Session* session = getSessionByID(request.getID());

    session->fd = fds[1];
    session->fdCGI = fds[0];
    session->size = 1000;
    session->chunk = 0;
    session->sizeChunked = 0;

    int ret = writeChunk(session->readBuf, session);

    if (ret < 0) {
        close(fds[0]);
        close(fds[1]);
        closeSessionByID(request.getID());
    }
    if (ret == 0) {
        close(fds[1]);
        session->bind(&HTTP::cgiCaller);
        return;
    }

    session->bind(&HTTP::recvChunkedToCGICallback);
    enableReadEvent(request.getID());
    disableWriteEvent(request.getID());
}

void HTTP::recvCGI(Request& request, Response& response)
{
    int fds[2];

    if (::pipe(fds) == -1) {
        throw httpEx<InternalServerError>("HTTP::Cannot open pipe!");
    }

    Session* session = getSessionByID(request.getID());

    session->fd = fds[1];
    session->fdCGI = fds[0];

    if (request.hasHeader("Content-Length")) {
        session->size = utils::to_number<size_t>(request.getHeaderValue("Content-Length"));
    }
    else {
        LOG_INFO("HTTP request 'Content-Length' header not found!\n");
        session->size = 0;
    }

    std::string& rbuf = session->readBuf;
    size_t bytes = std::min<size_t>(rbuf.size(), session->size);

    ssize_t writeBytes = writeFromBuf(session->fd, rbuf, bytes);

    if (writeBytes < 0) {
        close(session->fd);
        close(session->fdCGI);
        closeSessionByID(request.getID());
        LOG_ERROR("Dont write to socket: %d. Close connection.\n", socket);
        return;
    }

    session->size -= writeBytes;
    if (session->size == 0) {
        session->bind(&HTTP::cgiCaller);
        disableReadEvent(request.getID());
        enableWriteEvent(request.getID());
        return;
    }


    session->bind(&HTTP::recvCGICallback);
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

    session->fd = fd;
    session->size = 0;
    session->chunk = 0;
    session->sizeChunked = 0;

    session->bind(&HTTP::sendChunkedCallback);
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
