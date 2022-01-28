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
        s.bind(&HTTP::defaultReadCallback);
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

    if (flags & EventPool::M_EOF) {
        Session *session = getSessionByID(socket);
        if (session == nullptr) {
            LOG_ERROR("Dont find session to socket: %d. Close connection.\n", socket);
            closeSessionByID(socket);
            return;
        }
        (this->*session->func)(socket, session);
    }

    if (flags & EventPool::M_ERROR) {
        LOG_ERROR("Event error or eof, socket: %d\n", socket);
        closeSessionByID(socket);
    }
    if (flags & EventPool::M_TIMER) {
        LOG_ERROR("Event timeout, socket: %d\n", socket);
        closeSessionByID(socket);
    }
}




////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////




ssize_t readToBuf(int fd, std::string& rBuf)
{
    std::string buf;
    buf.resize(65536);

    ssize_t readBytes = ::read(fd, &buf[0], buf.size());
    if (readBytes < 0) {
        return readBytes;
    }

    rBuf.append(buf, 0, readBytes);

    return readBytes;
}

ssize_t writeFromBuf(int fd, std::string& wBuf, size_t nBytes)
{
    nBytes = std::min<size_t>(wBuf.size(), nBytes);
    nBytes = std::min<size_t>(nBytes, 65536);
    ssize_t writeBytes = ::write(fd, wBuf.c_str(), nBytes);
    if (writeBytes < 0) {
        return writeBytes;
    }

    wBuf.erase(0, writeBytes);

    return writeBytes;
}

void HTTP::defaultReadCallback(int socket, Session *session)
{
    LOG_DEBUG("defaultReadFunc call\n");

    std::string& rbuf = session->readBuf;


    if (readToBuf(socket, rbuf) < 0) {
        closeSessionByID(socket);
        LOG_ERROR("Dont read to socket: %d. Close connection.\n", socket);
        return;
    }

    session->bind(&HTTP::defaultWriteCallback);
    enableWriteEvent(socket);
    disableReadEvent(socket);
}

void HTTP::readBodyEventRead(int socket, Session* session)
{
    std::string& rbuf = session->readBuf;

    if (readToBuf(socket, rbuf) < 0) {
        closeSessionByID(socket);
        LOG_ERROR("Dont read to socket: %d. Close connection.\n", socket);
        return;
    }

    session->bind(&HTTP::readBodyEventWrite);
    enableWriteEvent(socket);
    disableReadEvent(socket);
}

void HTTP::readBodyEventWrite(int socket, Session* session)
{
    std::string& rbuf = session->readBuf;

    size_t size = std::min<size_t>(rbuf.size(), session->size);


    session->request.setBody(session->request.getBody() + std::string(rbuf, 0, size));
    rbuf.erase(0, size);
    session->size -= size;

    if (session->size == 0) {
        session->bind(&HTTP::handlerCallback);
        return;
    }


    session->bind(&HTTP::readBodyEventRead);
    // включаем read
    enableReadEvent(socket);
    disableWriteEvent(socket);
}

void HTTP::readBodyChunkedEventRead(int socket, Session* session)
{
    LOG_DEBUG("readBodyChunkedEventRead call\n");
    std::string& rbuf = session->readBuf;

    if (readToBuf(socket, rbuf) < 0) {
        closeSessionByID(socket);
        LOG_ERROR("Dont read to socket: %d. Close connection.\n", socket);
        return;
    }

    session->bind(&HTTP::readBodyChunkedEventWrite);
    enableWriteEvent(socket);
    disableReadEvent(socket);
}

void HTTP::readBodyChunkedEventWrite(int socket, Session* session)
{
    LOG_DEBUG("readBodyChunkedEventWrite call\n");
    std::string& rbuf = session->readBuf;

    while (!rbuf.empty())
    {
        if (session->chunk == 0) {
            rbuf.erase(0, rbuf.find_first_not_of("\r\n"));
            size_t found = rbuf.find("\n");
            if (found != std::string::npos) {
                size_t pos = rbuf.find_first_not_of("\r\n", found);
                session->chunk = utils::to_hex_number<size_t>(std::string(rbuf, 0, pos));
                rbuf.erase(0, pos);
            }
            else {
                break;
            }
        }

        if (session->chunk == 0) {
            session->bind(&HTTP::handlerCallback);
            return;
        }

        size_t bytes = std::min<size_t>(rbuf.size(), session->chunk);
        session->request.setBody(session->request.getBody() + std::string(rbuf, 0, bytes));
        session->sizeChunked += bytes;
        rbuf.erase(0, bytes);
        session->chunk -= bytes;
    }

    session->bind(&HTTP::readBodyChunkedEventRead);
    enableReadEvent(socket);
    disableWriteEvent(socket);
}

void HTTP::handlerCallback(int, Session *session)
{
    Request& request = session->request;

    Response response;
    handler(request, response);

    std::string& wbuf = session->writeBuf;

    wbuf.append(response.getContent());

    session->bind(&HTTP::doneWriteCallback);
}

void HTTP::defaultWriteCallback(int socket, Session *session)
{
    LOG_DEBUG("defaultWriteFunc call\n");


    std::string& rbuf = session->readBuf;

    size_t foundNN = rbuf.find("\n\n");
    size_t foundRN = rbuf.find("\r\n\r\n");

    size_t found = std::min<size_t>(foundNN, foundRN);
    if (found != std::string::npos) {
        size_t pos = rbuf.find_first_not_of("\r\n", found);
        session->request.reset();
        session->request.parse(std::string(rbuf, 0, pos));
        rbuf.erase(0, pos);


        if (session->request.hasHeader("Content-Length")) {
            session->size = utils::to_number<size_t>(session->request.getHeaderValue("Content-Length"));
            session->bind(&HTTP::readBodyEventWrite);
            return;
        }

        if (session->request.hasHeader("Transfer-Encoding") && session->request.getHeaderValue("Transfer-Encoding") == "chunked") {
            session->chunk = 0;
            session->sizeChunked = 0;
            session->bind(&HTTP::readBodyChunkedEventWrite);
            return;
        }

        session->bind(&HTTP::handlerCallback);

    }
    else {
        session->bind(&HTTP::defaultReadCallback);
        enableReadEvent(socket);
        disableWriteEvent(socket);
    }
}

void HTTP::doneWriteCallback(int socket, Session *session)
{
    LOG_DEBUG("doneFunc call\n");
    std::string& wbuf = session->writeBuf;

    if (wbuf.empty()) {
        session->bind(&HTTP::defaultWriteCallback);
    }
    else {
        if (writeFromBuf(socket, wbuf, wbuf.size()) < 0) {
            closeSessionByID(socket);
            LOG_ERROR("Dont write to socket: %d. Close connection.\n", socket);
            return;
        }
    }
}
