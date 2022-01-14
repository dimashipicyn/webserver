#include <iostream>
#include "Logger.h"
#include "http.h"
#include "EventPool.h"
#include "Request.h"
#include "Response.h"
#include "Autoindex.hpp"
#include "Route.hpp"
#include "Route.hpp"
#include "Server.hpp"
#include "utils.h"

struct Session
{
    Session()
        : conn()
        , host()
        , writeBuffer()
        , readBuffer()
    {

    }
    Session(const tcp::tdSocketAddressPair& conn)
        : conn(conn)
        , host()
        , writeBuffer()
        , readBuffer()
    {

    };
    Session(Session& session)
        : conn(session.conn)
        , host()
        , writeBuffer()
        , readBuffer()
    {
    };
    Session& operator=(Session& session) {
        if (&session == this) {
            return *this;
        }
        conn = session.conn;
        writeBuffer = session.writeBuffer;
        readBuffer = session.readBuffer;
        return *this;
    };
    virtual ~Session() {

    };

    tcp::tdSocketAddressPair    conn;
    std::string         host;
    std::string         writeBuffer;
    std::string         readBuffer;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HTTP::HTTP()
    : sessionMap_()
{

}

HTTP::~HTTP()
{

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
    enableReadEvent(session.conn.first);

    sessionMap_[id] = session;
}

void HTTP::asyncAccept(int socket)
{
    LOG_DEBUG("Event accepter call\n");

    try
    {
        Session *session = getSessionByID(socket);
        if (session == nullptr) {
            LOG_ERROR("Dont find session to socket: %d. Close.\n", socket);
            close(socket);
            return;
        }

        tcp::tdSocketAddressPair conn = tcp::accept(session->conn);

        LOG_DEBUG("New connect fd: %d\n", conn.first);

        Session s(conn);
        newSessionByID(conn.first, s);
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

// здесь происходит обработка запроса
void HTTP::handler(Request& request, Response& response)
{
    LOG_DEBUG("Http handler call\n");
    LOG_DEBUG("--------------PRINT REQUEST--------------\n");
    std::cout << request << std::endl;

	// Сравниваем расширение запрошенного ресурса с cgi расширением для этого локейшена. Если бьется, запуск скрипта
	SettingsManager *settingsManager = SettingsManager::getInstance();
    Server *server = settingsManager->findServer("127.0.0.1", 1234); // !!Здесь нужно передавать ip:port сервера, обрабатывающего запрос. Пока костыль
	std::string path = request.getPath();
	Route *route = server == nullptr ? nullptr : server->findRouteByPath(path);
	if (route != nullptr && utils::getExtension(path) == route->getCgi()) {
		response.setContent(Cgi(request).runCGI());
	}
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


//    if (request.getMethod() == "GET" && request.getPath() == "/") {
//        std::stringstream ss;
//        std::string s("Hello Webserver!\n");
//        ss << "HTTP/1.1 200 OK\n"
//           << "Content-Length: " << s.size() << "\n"
//           << "Content-Type: text/html\n\n";
//        response.setContent(ss.str() + s);
//    }
}


void HTTP::start() {
    EventPool::start();
}

void HTTP::listen(const std::string& host)
{
    try {
        tcp::tdSocketAddressPair conn = tcp::tcpSocket(host);
        tcp::listen(conn);
        tcp::makeNonBlock(conn);

        Session listenSession(conn);
        newListenerEvent(conn.first);
        sessionMap_[conn.first] = listenSession;
    }  catch (std::exception& e) {
        LOG_ERROR("HTTP: %s\n", e.what());
    }
}
