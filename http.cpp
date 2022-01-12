#include <iostream>
#include "Logger.h"
#include "http.h"
#include "EventPool.h"
#include "Request.h"
#include "Response.h"

struct Session : public ISession
{
    Session() {

    };
    virtual ~Session() {

    };
    virtual void asyncEvent(std::uint16_t flags)
    {
        LOG_DEBUG("Event handler call\n");
        if ( flags & EventPool::M_EOF  || flags & EventPool::M_ERROR ) {
            LOG_DEBUG("Event error or eof, fd: %d\n", socket->getSock());
            close();
        }
    };

    virtual void asyncRead(char *buffer, int64_t bytes)
    {
        LOG_DEBUG("Event reader call\n");

        /*
        int nBytes = ::read(event->sock, buffer, 1024);
        if (nBytes < 0) {
            LOG_ERROR("Error read socket: %d. %s\n", event->sock, ::strerror(errno));
        }
        buffer[nBytes] = '\0';
        request.parse(buffer);

        if (request.getState() == Request::PARSE_ERROR
         || request.getState() == Request::PARSE_DONE)
        {
            evPool->addEvent(event, EventPool::M_WRITE
                                  | EventPool::M_ADD
                                  | EventPool::M_ENABLE);
            //evPool->eventSetFlags(EventPool::M_TIMER
            //                      | EventPool::M_ADD
             //                     | EventPool::M_ENABLE
            //                      , 1000
            //                      );
        }
        */
    };

    virtual void write(char *buffer, int64_t bytes)
    {
        LOG_DEBUG("Event writer call\n");

        /*
        http.handler(reader->request, response);

        // пишем в сокет
        int sock = event->sock;
        ::write(sock, response.getContent().c_str(), response.getContent().size());

        // сброс
        reader->request.reset();
        //response.reset();


        // выключаем write
        evPool->addEvent(event, EventPool::M_WRITE | EventPool::M_DISABLE);
        //evPool->eventSetFlags(EventPool::M_TIMER | EventPool::M_DISABLE);
        */
    };

    virtual void accept(EventPool& evPool)
    {
        LOG_DEBUG("Event accepter call\n");

        try
        {
            TcpSocket conn = socket->accept();
            LOG_DEBUG("New connect fd: %d\n", conn.getSock());

            Session clientSession;
            clientSession.socket.reset(new TcpSocket(conn));
            evPool.newSession(clientSession, EventPool::M_READ|EventPool::M_ADD|EventPool::M_ENABLE);
        }
        catch (std::exception& e)
        {
            LOG_ERROR("Session error: %s\n", e.what());
        }
    };
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HTTP::HTTP()
{

}

HTTP::~HTTP()
{

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
    evPool_.start();
}

void HTTP::listen(const std::string& host) {
    try {
        TcpSocket sock(host);
        sock.makeNonBlock();
        sock.listen();

        Session listenSession;
        listenSession.socket.reset(new TcpSocket(sock));
        evPool_.newListenSession(listenSession);
    }  catch (std::exception& e) {
        LOG_ERROR("HTTP: %s\n", e.what());
    }
}
