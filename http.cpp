#include <iostream>
#include "Logger.h"
#include "http.h"
#include "EventPool.h"
#include "Request.h"
#include "Response.h"

struct Session
{
    Session()
        : socket_(nullptr)
        , writeBuffer_()
        , readBuffer_()
    {

    }
    Session(std::auto_ptr<TcpSocket>& socket)
        : socket_(socket)
    {

    };
    Session(Session& session)
        : socket_(session.socket_)
    {
    };
    Session& operator=(Session& session) {
        if (&session == this) {
            return *this;
        }
        socket_ = session.socket_;
        return *this;
    };
    virtual ~Session() {

    };


    std::auto_ptr<TcpSocket>    socket_;
    std::string                 writeBuffer_;
    std::string                 readBuffer_;
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

void HTTP::newSession(std::auto_ptr<TcpSocket>& socket)
{
    Session clientSession(socket);
    enableReadEvent(clientSession.socket_->getSock());

    sessionMap_[clientSession.socket_->getSock()] = clientSession;
    //sessionMap_.insert(std::pair<int, Session>(clientSession.socket_->getSock(), clientSession));
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

        std::auto_ptr<TcpSocket> conn(new TcpSocket(session->socket_->accept()));
        LOG_DEBUG("New connect fd: %d\n", conn->getSock());

        newSession(conn);
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

    request.parse(session->readBuffer_.c_str());
    request.setHost(session->socket_->getHost());

    session->readBuffer_.clear();
    handler(request, response);

    session->writeBuffer_.append(response.getContent());
    std::string& writeBuffer = session->writeBuffer_;
    int64_t writeBytes = session->socket_->write(writeBuffer.c_str(), writeBuffer.size());
    session->writeBuffer_.erase(0, writeBytes);

    // выключаем write
    disableWriteEvent(session->socket_->getSock());
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

    int64_t readBytes = session->socket_->read(&buffer[0], bufferSize - 1);
    if (readBytes < 0) {
        close(socket);
        LOG_ERROR("Dont read to socket: %d. Close connection.\n", socket);
    }

    session->readBuffer_.append(buffer.c_str());

    // включаем write
    enableWriteEvent(session->socket_->getSock());
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

    if (request.getMethod() == "GET" && request.getPath() == "/") {
        std::stringstream ss;
        std::string s("Hello Webserver!\n");
        ss << "HTTP/1.1 200 OK\n"
           << "Content-Length: " << s.size() << "\n"
           << "Content-Type: text/html\n\n";
        response.setContent(ss.str() + s);
    }
}


void HTTP::start() {
    EventPool::start();
}

void HTTP::listen(const std::string& host)
{
    try {
        std::auto_ptr<TcpSocket> sock(new TcpSocket(host));
        sock->listen();
        sock->makeNonBlock();

        Session listenSession(sock);
        newListenerEvent(listenSession.socket_->getSock());
        sessionMap_[listenSession.socket_->getSock()] = listenSession;
    }  catch (std::exception& e) {
        LOG_ERROR("HTTP: %s\n", e.what());
    }
}
