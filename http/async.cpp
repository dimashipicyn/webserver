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

    int         fds[2];

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
    if (flags & EventPool::M_EOF || flags & EventPool::M_ERROR) {
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
    const size_t bufSize = (1 << 16);
    char buf[bufSize];

    ssize_t readBytes = ::read(fd, buf, bufSize);
    if (readBytes < 0) {
        return readBytes;
    }

    rBuf.append(buf, buf + readBytes);

    return readBytes;
}

ssize_t writeFromBuf(int fd, std::string& wBuf, size_t nBytes)
{
    ssize_t writeBytes = ::write(fd, wBuf.c_str(), nBytes);
    if (writeBytes < 0) {
        return writeBytes;
    }

    wBuf.erase(0, writeBytes);

    return writeBytes;
}

static int32_t writeChunk(std::string& rbuf, Session* session);


void HTTP::defaultReadCallback(int socket, Session *session)
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


        enableWriteEvent(socket);
        disableReadEvent(socket);

        if (session->request.hasHeader("Content-Length")) {
            pipe(session->fds);
            session->size = utils::to_number<size_t>(session->request.getHeaderValue("Content-Length"));
            session->bind(&HTTP::readBodyEventWrite);
            return;
        }

        if (session->request.hasHeader("Transfer-Encoding") && session->request.getHeaderValue("Transfer-Encoding") == "chunked") {
            pipe(session->fds);
            session->chunk = 0;
            session->sizeChunked = 0;
            session->bind(&HTTP::readBodyChunkedEventWrite);
            return;
        }

        session->bind(&HTTP::defaultWriteCallback);
    }
}

void HTTP::readBodyEventRead(int socket, Session* session)
{
    std::string& rbuf = session->readBuf;

    if (readToBuf(socket, rbuf) < 0) {
        close(session->fds[0]);
        close(session->fds[1]);
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

    ssize_t bytes = writeFromBuf(session->fds[1], rbuf, size);
    if (bytes < 0) {
        close(session->fds[0]);
        close(session->fds[1]);
        closeSessionByID(socket);
    }

    session->size -= bytes;

    if (session->size == 0) {
        close(session->fds[1]);
        session->bind(&HTTP::defaultWriteCallback);
        return;
    }
    if (rbuf.empty()) {
        session->bind(&HTTP::readBodyEventRead);
        // включаем write
        enableReadEvent(socket);
        disableWriteEvent(socket);
    }
}

void HTTP::readBodyChunkedEventRead(int socket, Session* session)
{
    std::string& rbuf = session->readBuf;

    if (readToBuf(socket, rbuf) < 0) {
        close(session->fds[0]);
        close(session->fds[1]);
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
    std::string& rbuf = session->readBuf;

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
                break;
            }
        }


        if (session->chunk == 0) {
            session->bind(&HTTP::defaultWriteCallback);
            return;
        }


        while (!rbuf.empty() && session->chunk > 0) {
            ssize_t writeBytes = writeFromBuf(session->fds[1], rbuf, session->chunk);
            if (writeBytes < 0) {
                close(session->fds[0]);
                close(session->fds[1]);
                closeSessionByID(socket);
            }

            session->chunk -= writeBytes;
            session->sizeChunked += writeBytes;
        }
    }
    session->bind(&HTTP::readBodyChunkedEventRead);
    enableReadEvent(socket);
    disableWriteEvent(socket);
}


bool HTTP::saveToFile(int fd, const std::string& path)
{
    std::ofstream file(path);
    if (file.is_open()) {
        char buf[1 << 16];
        ssize_t bytes;
        while ((bytes = read(fd, buf, 1 << 16)) > 0) {
            buf[bytes] = '\0';
            file << buf;
        }
        if (bytes == 0) {
            return true;
        }
    }
    return false;
}

void HTTP::defaultWriteCallback(int socket, Session *session)
{
    LOG_DEBUG("defaultWriteFunc call\n");
    session->bind(&HTTP::doneWriteCallback);

    Request& request = session->request;
    request.fd = session->fds[0];

    close(session->fds[1]);
    Response response;
    handler(request, response);

    close(session->fds[0]);

    std::string& wbuf = session->writeBuf;
    LOG_DEBUG("REDIRECTION HERE\n");
    std::cout << response.getContent();

    wbuf.append(response.getContent());
}

void HTTP::doneWriteCallback(int socket, Session *session)
{
    LOG_DEBUG("doneFunc call\n");
    std::string& wbuf = session->writeBuf;

    if (wbuf.empty()) {
        disableWriteEvent(socket);
        enableReadEvent(socket);
        session->bind(&HTTP::defaultReadCallback);
    }
    else {
        if (writeFromBuf(socket, wbuf, wbuf.size()) < 0) {
            closeSessionByID(socket);
            LOG_ERROR("Dont write to socket: %d. Close connection.\n", socket);
            return;
        }
    }
}

void HTTP::sendFileEventWrite(int socket, Session *session)
{
    LOG_DEBUG("sendFileFunc call\n");

    std::string& wbuf = session->writeBuf;

    ssize_t readBytes = readToBuf(session->fd, wbuf);
    if (readBytes < 0) {
        close(session->fd);
        close(session->fds[0]);
        close(session->fds[1]);
        closeSessionByID(socket);
        LOG_ERROR("Dont read to file: %d. Close connection.\n", socket);
        return;
    }

    ssize_t writeBytes = writeFromBuf(socket, wbuf, wbuf.size());
    if (writeBytes < 0) {
        close(session->fd);
        close(session->fds[0]);
        close(session->fds[1]);
        closeSessionByID(socket);
        LOG_ERROR("Dont write to socket: %d. Close connection.\n", socket);
        return;
    }

    if (wbuf.empty()) {
        session->bind(&HTTP::doneWriteCallback);
        enableReadEvent(socket);
        disableWriteEvent(socket);
    }
}

void HTTP::sendFileChunkedEventWrite(int socket, Session *session)
{
    LOG_DEBUG("sendFileFunc call\n");

    std::string buf;

    waitpid(-1, nullptr, 0);
    ssize_t readBytes = readToBuf(session->fd, buf);
    if (readBytes < 0) {
        close(session->fd);
        close(session->fds[0]);
        close(session->fds[1]);
        closeSessionByID(socket);
        LOG_ERROR("Dont read to file: %d. Close connection.\n", socket);
        return;
    }

    std::string& wbuf = session->writeBuf;
    wbuf += (utils::to_string<ssize_t>(readBytes) + "\r\n");
    wbuf.append(buf + "\r\n");

    ssize_t writeBytes = writeFromBuf(socket, wbuf, wbuf.size());
    if (writeBytes < 0) {
        close(session->fd);
        close(session->fds[0]);
        close(session->fds[1]);
        closeSessionByID(socket);
        LOG_ERROR("Dont write to socket: %d. Close connection.\n", socket);
        return;
    }

    if (readBytes == 0) {
        session->bind(&HTTP::doneWriteCallback);
        enableReadEvent(socket);
        disableWriteEvent(socket);
    }
}


void HTTP::sendFile(const Request& request, Response& response, const std::string& path, bool chunked)
{
    int fd = ::open(path.c_str(), O_RDONLY);

    if (fd == -1) {
        throw httpEx<InternalServerError>("Cannot open file: " + path);
    }

    Session* session = getSessionByID(request.getID());

    response.setStatusCode(200);

    session->fd = fd;

    if (chunked) {
        response.setHeaderField("Transfer-Encoding", "chunked");
        session->bind(&HTTP::sendFileChunkedEventWrite);
    }
    else {
        struct stat info;

        if (::fstat(fd, &info) == -1) {
            throw httpEx<InternalServerError>("Cannot stat file info: " + path);
        }

        response.setHeaderField("Content-Length", info.st_size);

        session->size = info.st_size;
        session->bind(&HTTP::sendFileEventWrite);
    }

}

void HTTP::sendFile(const Request& request, Response& response, int fd, bool chunked)
{
    Session* session = getSessionByID(request.getID());

    response.setStatusCode(200);

    session->fd = fd;

    if (chunked) {
        response.setHeaderField("Transfer-Encoding", "chunked");
        session->bind(&HTTP::sendFileChunkedEventWrite);
    }
    else {
        struct stat info;

        if (::fstat(fd, &info) == -1) {
            throw httpEx<InternalServerError>("Cannot stat file info: %d" + utils::to_string<int>(fd));
        }

        response.setHeaderField("Content-Length", info.st_size);

        session->size = info.st_size;
        session->bind(&HTTP::sendFileEventWrite);
    }
}


/*
void HTTP::recvFileCallback(int socket, Session *session)
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
        session->bind(&HTTP::doneWriteCallback);
        disableReadEvent(socket);
        enableWriteEvent(socket);
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

void HTTP::recvChunkedFileCallback(int socket, Session *session)
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
        session->bind(&HTTP::doneWriteCallback);
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
*



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
        session->bind(&HTTP::doneWriteCallback);
        close(fd);
    }

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
    session->bind(&HTTP::sendFileCallback);
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


    session->bind(&HTTP::recvFileCallback);
    enableReadEvent(request.getID());
    disableWriteEvent(request.getID());
}

void HTTP::recvFileChunked(const Request& request, Response& response, const std::string& path)
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

    session->bind(&HTTP::recvChunkedFileCallback);
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


///////////////////////// CGI ////////////////////////////////



void HTTP::doneWriteCGICallback(int socket, Session *session)
{
    LOG_DEBUG("doneFunc call\n");
    std::string& wbuf = session->writeBuf;

    if (wbuf.empty()) {
        closeSessionByID(socket);
    }
    else {
        if (writeFromBuf(session->fd, wbuf, wbuf.size()) < 0) {
            close(session->fd);
            closeSessionByID(socket);
            LOG_ERROR("Dont write to socket: %d. Close connection.\n", socket);
            return;
        }
    }
}

std::pair<int, int> HTTP::sendCGI(const Request& request)
{
    int fds[2];

    if (::pipe(fds) == -1) {
        throw httpEx<InternalServerError>("HTTP::Cannot open pipe!");
    }

    Session s;
    s.bind(&HTTP::sendFileCallback);
    s.fd = request.getID();
    s.size = 1024;

    newSessionByID(fds[0], s);
    enableReadEvent(fds[0]);
    enableTimerEvent(fds[0], sessionTimeout);

    return std::pair<int, int>(fds[0], fds[1]);
}

std::pair<int, int> HTTP::sendCGIChunked(const Request& request)
{
    int fds[2];

    if (::pipe(fds) == -1) {
        throw httpEx<InternalServerError>("HTTP::Cannot open pipe!");
    }

    Session s;
    s.bind(&HTTP::sendCGIChunkedCallback);
    s.fd = request.getID();
    s.size = 1024;

    newSessionByID(fds[0], s);
    enableReadEvent(fds[0]);
    enableTimerEvent(fds[0], sessionTimeout);

    return std::pair<int, int>(fds[0], fds[1]);
}

void HTTP::sendCGIChunkedCallback(int socket, Session *session)
{
    std::string& rbuf = session->readBuf;

    ssize_t readBytes = readToBuf(socket, rbuf);
    if (readBytes < 0) {
        close(session->fd);
        closeSessionByID(socket);
        LOG_ERROR("Dont read to file: %d. Close connection.\n", socket);
        return;
    }

    std::string& wbuf = session->writeBuf;
    wbuf.append(utils::to_string<int32_t>(readBytes));
    wbuf.append("\r\n");
    wbuf.append(rbuf, 0, readBytes);
    wbuf.append("\r\n");

    ssize_t writeBytes = writeFromBuf(session->fd, wbuf, wbuf.size());

    if (writeBytes < 0) {
        close(session->fd);
        closeSessionByID(socket);
        LOG_ERROR("Dont write to socket: %d. Close connection.\n", socket);
        return;
    }

    int status;
    waitpid(-1, &status, WNOHANG);
    if (WIFEXITED(status)) {
        wbuf.append("0\r\n");
        disableReadEvent(socket);
        enableWriteEvent(socket);
        session->bind(&HTTP::doneWriteCGICallback);
    }
}

std::pair<int, int> HTTP::recvCGI(const Request& request)
{
    int fds[2];

    if (::pipe(fds) == -1) {
        throw httpEx<InternalServerError>("HTTP::Cannot open pipe!");
    }

    Session* session = getSessionByID(request.getID());

    session->fd = fds[1];
    //session->fdCGI = fds[0];

    session->size = utils::to_number<size_t>(request.getHeaderValue("Content-Length"));

    std::string& rbuf = session->readBuf;
    size_t bytes = std::min<size_t>(rbuf.size(), session->size);

    ssize_t writeBytes = writeFromBuf(session->fd, rbuf, bytes);

    if (writeBytes < 0) {
        close(session->fd);
        closeSessionByID(request.getID());
        LOG_ERROR("Dont write to socket: %d. Close connection.\n", socket);
        return std::pair<int, int>(-1, -1); // TODO add throw except
    }

    session->size -= writeBytes;
    if (session->size == 0) {
        close(session->fd);
        session->bind(&HTTP::defaultReadCallback);
        return std::pair<int, int>(fds[0], fds[1]);
    }


    session->bind(&HTTP::recvCGICallback);
    enableReadEvent(request.getID());
    disableWriteEvent(request.getID());
    return std::pair<int, int>(fds[0], fds[1]);
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
        //session->bind(&HTTP::cgiCaller);
        return;
    }

    session->bind(&HTTP::recvChunkedCGICallback);
    enableReadEvent(request.getID());
    disableWriteEvent(request.getID());
}

void HTTP::recvChunkedCGICallback(int socket, Session *session)
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
        //session->bind(&HTTP::cgiCaller);
        enableWriteEvent(socket);
        disableReadEvent(socket);
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
        close(session->fd);
        disableReadEvent(socket);
        enableWriteEvent(socket);
        session->bind(&HTTP::doneWriteCallback);
        return;
    }
}

*/
