#include <iostream>
#include <array>
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

typedef void (HTTP::*handlerFunc)(int, Session*);

struct RequestState {
    RequestState()
        : state(GET_REQUEST)
        , readBuf()
        , writeBuf()
        , funcs()
    {
        funcs[GET_REQUEST] = &HTTP::defaultReadFunc;
        funcs[GET_RESPONSE] = &HTTP::defaultWriteFunc;
        funcs[DONE] = &HTTP::doneFunc;
    }

    handlerFunc func() {
        return funcs[state];
    }
    enum State {
        GET_REQUEST,
        GET_RESPONSE,
        DONE,
        Total
    };
    State       state;
    std::string readBuf;
    std::string writeBuf;
    std::array<handlerFunc, Total> funcs;
};


struct Session
{
    Session()
        : host()
        , reqState()
    {
    }
    Session(const std::string& host)
        : host(host)
        , reqState()
    {
    };
    Session(Session& session)
        : host(session.host)
        , reqState()
    {
    };
    Session& operator=(Session& session) {
        if (&session == this) {
            return *this;
        }
        host = session.host;
        reqState = session.reqState;
        return *this;
    };
    ~Session() {

    };

    std::string         host;
    RequestState        reqState;
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

        Session s(conn.getHost());
        newSessionByID(conn.getSock(), s);
        enableReadEvent(conn.getSock());
        enableTimerEvent(conn.getSock(), 20000); // TODO add config
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
    (this->*session->reqState.func())(socket, session);
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
    (this->*session->reqState.func())(socket, session);
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
    const int64_t bufSize = (1 << 16);
    char buf[bufSize];

    int64_t readBytes = ::read(socket, buf, bufSize - 1);
    if (readBytes < 0) {
        closeSessionByID(socket);
        LOG_ERROR("Dont read to socket: %d. Close connection.\n", socket);
        return;
    }
    buf[readBytes] = '\0';
    session->reqState.readBuf.append(buf);
    if (session->reqState.readBuf.find("\n\n") != std::string::npos
    || session->reqState.readBuf.find("\r\n\r\n") != std::string::npos)
    {
        session->reqState.state = RequestState::GET_RESPONSE;
        // включаем write
        enableWriteEvent(socket);
        disableReadEvent(socket);
    }
}

void HTTP::defaultWriteFunc(int socket, Session *session)
{
    Request request;
    Response response;

    session->reqState.state = RequestState::DONE;

    std::string& rbuf = session->reqState.readBuf;
    request.parse(rbuf);
    rbuf.clear();

    request.setHost(session->host);
    request.setID(socket);
    handler(request, response);

    std::string& wbuf = session->reqState.writeBuf;
    wbuf.append(response.getContent());

    int64_t writeBytes = ::write(socket, wbuf.c_str(), wbuf.size());

    if (writeBytes < 0) {
        closeSessionByID(socket);
        LOG_ERROR("Dont write to socket: %d. Close connection.\n", socket);
        return;
    }

    wbuf.erase(0, writeBytes);
}

void HTTP::doneFunc(int socket, Session *session)
{
    std::string& wbuf = session->reqState.writeBuf;

    if (wbuf.empty()) {
        disableWriteEvent(socket);
        enableReadEvent(socket);
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
void HTTP::handler(Request& request, Response& response)
{
    try {
        LOG_DEBUG("Http handler call\n");
        LOG_DEBUG("--------------PRINT REQUEST--------------\n");
        std::cout << request << std::endl;

        // Сравниваем расширение запрошенного ресурса с cgi расширением для этого локейшена. Если бьется, запуск скрипта
        SettingsManager *settingsManager = SettingsManager::getInstance();
        Server *server = settingsManager->findServer(request.getHost());
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
