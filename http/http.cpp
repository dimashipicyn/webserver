#include <iostream>
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

void HTTP::cgi(Request &request, Response& response, Route* route) {
    const std::string& path = request.getPath();

    if (route != nullptr && utils::getExtension(path) == route->getCgi()) {
        response.setContent(Cgi(request, *route).runCGI());
    }
}

void HTTP::autoindex(Request &request, Response &response, Route *route) {
    const std::string& path = request.getPath();

    if (route != nullptr && route->isAutoindex() && utils::getExtension(path).empty()) {
        std::stringstream header;
        std::string html;
        try {
            html = Autoindex(*route).generatePage(path);
        } catch (std::runtime_error &e) {
            LOG_ERROR("%s\n", e.what());
            // Здесь респонс дефолтной ошибкой или той что указана в конфиге
            return;
        }
        header << "HTTP/1.1 200 OK\n"
        << "Content-Length: " << html.size() << "\n"
        << "Content-Type: text/html\n\n";
        response.setContent(header.str() + html);
    }
}

// здесь происходит обработка запроса
void HTTP::handler(Request& request, Response& response)
{
    try {
        LOG_DEBUG("Http handler call\n");
        LOG_DEBUG("--------------PRINT REQUEST--------------\n");
        std::cout << request << std::endl;

        // Сравниваем расширение запрошенного ресурса с cgi расширением для этого локейшена. Если бьется, запуск скрипта
        SettingsManager *settingsManager = SettingsManager::getInstance();
        Server *server = settingsManager->findServer("127.0.0.1", 1234); // !!Здесь нужно передавать ip:port сервера, обрабатывающего запрос. Пока костыль
        Route *route = server == nullptr ? nullptr : server->findRouteByPath(request.getPath());

        cgi(request, response, route);
        autoindex(request, response, route);
    }
    catch (httpEx<BadRequest>& e) {
        LOG_INFO("BadRequest: %s\n", e.what());
    }
    catch (httpEx<Unauthorized>& e) {
        LOG_INFO("Unauthorized: %s\n", e.what());
    }
    catch (httpEx<PaymentRequired>& e) {
        LOG_INFO("PaymentRequired: %s\n", e.what());
    }
    catch (httpEx<Forbidden>& e) {
        LOG_INFO("Forbidden: %s\n", e.what());
    }
    catch (httpEx<NotFound>& e) {
        LOG_INFO("NotFound: %s\n", e.what());
    }
    catch (httpEx<MethodNotAllowed>& e) {
        LOG_INFO("MethodNotAllowed: %s\n", e.what());
    }
    catch (httpEx<NotAcceptable>& e) {
        LOG_INFO("NotAcceptable: %s\n", e.what());
    }
    catch (httpEx<ProxyAuthenticationRequired>& e) {
        LOG_INFO("ProxyAuthenticationRequired: %s\n", e.what());
    }
    catch (httpEx<RequestTimeout>& e) {
        LOG_INFO("RequestTimeout: %s\n", e.what());
    }
    catch (httpEx<Conflict>& e) {
        LOG_INFO("Conflict: %s\n", e.what());
    }
    catch (httpEx<Gone>& e) {
        LOG_INFO("Gone: %s\n", e.what());
    }
    catch (httpEx<LengthRequired>& e) {
        LOG_INFO("LengthRequired: %s\n", e.what());
    }
    catch (httpEx<PreconditionFailed>& e) {
        LOG_INFO("PreconditionFailed: %s\n", e.what());
    }
    catch (httpEx<PayloadToLarge>& e) {
        LOG_INFO("PayloadToLarge: %s\n", e.what());
    }
    catch (httpEx<UriTooLong>& e) {
        LOG_INFO("UriTooLong: %s\n", e.what());
    }
    catch (httpEx<UnsupportedMediaType>& e) {
        LOG_INFO("UnsupportedMediaType: %s\n", e.what());
    }
    catch (httpEx<RangeNotSatisfiable>& e) {
        LOG_INFO("RangeNotSatisfiable: %s\n", e.what());
    }
    catch (httpEx<ExpectationFailed>& e) {
        LOG_INFO("ExpectationFailed: %s\n", e.what());
    }
    catch (httpEx<ImATeapot>& e) {
        LOG_INFO("ImATeapot: %s\n", e.what());
    }
    catch (httpEx<UnprocessableEntity>& e) {
        LOG_INFO("UnprocessableEntity: %s\n", e.what());
    }
    catch (httpEx<TooEarly>& e) {
        LOG_INFO("TooEarly: %s\n", e.what());
    }
    catch (httpEx<UpgradeRequired>& e) {
        LOG_INFO("UpgradeRequired: %s\n", e.what());
    }
    catch (httpEx<PreconditionRequired>& e) {
        LOG_INFO("PreconditionRequired: %s\n", e.what());
    }
    catch (httpEx<TooManyRequests>& e) {
        LOG_INFO("TooManyRequests: %s\n", e.what());
    }
    catch (httpEx<RequestHeaderFieldsTooLarge>& e) {
        LOG_INFO("RequestHeaderFieldsTooLarge: %s\n", e.what());
    }
    catch (httpEx<UnavailableForLegalReasons>& e) {
        LOG_INFO("UnavailableForLegalReasons: %s\n", e.what());
    }
    catch (httpEx<InternalServerError>& e) {
        LOG_INFO("InternalServerError: %s\n", e.what());
    }
    catch (httpEx<NotImplemented>& e) {
        LOG_INFO("NotImplemented: %s\n", e.what());
    }
    catch (httpEx<BadGateway>& e) {
        LOG_INFO("BadGateway: %s\n", e.what());
    }
    catch (httpEx<ServiceUnavailable>& e) {
        LOG_INFO("ServiceUnavailable: %s\n", e.what());
    }
    catch (httpEx<GatewayTimeout>& e) {
        LOG_INFO("GatewayTimeout: %s\n", e.what());
    }
    catch (httpEx<HTTPVersionNotSupported>& e) {
        LOG_INFO("HTTPVersionNotSupported: %s\n", e.what());
    }
    catch (httpEx<VariantAlsoNegotiates>& e) {
        LOG_INFO("VariantAlsoNegotiates: %s\n", e.what());
    }
    catch (httpEx<InsufficientStorage>& e) {
        LOG_INFO("InsufficientStorage: %s\n", e.what());
    }
    catch (httpEx<LoopDetected>& e) {
        LOG_INFO("LoopDetected: %s\n", e.what());
    }
    catch (httpEx<NotExtended>& e) {
        LOG_INFO("NotExtended: %s\n", e.what());
    }
    catch (httpEx<NetworkAuthenticationRequired>& e) {
        LOG_INFO("NetworkAuthenticationRequired: %s\n", e.what());
    }
}
