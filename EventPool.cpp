//
// Created by Lajuana Bespin on 10/29/21.
//

#include <unistd.h>
#include <sys/event.h>
#include <deque>
#include <map>
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include "Logger.h"
#include "EventPool.h"
#include "kqueue.h"
///
/// \brief webserv::EventPool::Event::Event
/// \param sock
/// \param addr
///
webserv::EventPool::Event::Event(int sock, struct sockaddr *addr)
    : sock(sock),
      addr(addr),
      acceptor(nullptr),
      reader(nullptr),
      writer(nullptr),
      handler(nullptr)
{

}

webserv::EventPool::Event::~Event() {
    ::close(sock);
}

///
/// \brief webserv::EventPool::Event::setCb set callBack classes
/// \param re - pointer to reader
/// \param wr - pointer to writer
/// \param hr - pointer to handler
///
void webserv::EventPool::Event::setCb(
        webserv::IEventAcceptor *acc,
        webserv::IEventReader *re,
        webserv::IEventWriter *wr,
        webserv::IEventHandler *hr)
{
    acceptor = acc;
    reader = re;
    writer = wr;
    handler = hr;
}

webserv::EventPool::EventPool()
: poll_(), listenSockets_()
{
}

webserv::EventPool::~EventPool() {

}

void webserv::EventPool::eventSetCb(
        webserv::IEventAcceptor *acc,
        webserv::IEventReader *reader,
        webserv::IEventWriter *writer,
        webserv::IEventHandler *handler
        ) {
    assert(event_);
    event_->setCb(acc, reader, writer, handler);
}

void webserv::EventPool::eventSetFlags(std::uint16_t flags, std::int64_t time) {
    poll_.setEvent(event_->sock, flags, event_, time);
}

int webserv::EventPool::eventGetSock() {
    return event_->sock;
}

void webserv::EventPool::eventLoop() {
    int sizeEvents = 1024;
    std::vector<Kqueue::ev> events(sizeEvents);

    webserv::logger.log(webserv::Logger::INFO, "Run event loop");

    while (true) {
        int n = poll_.getEvents(events);

        for (int i = 0; i < n; ++i)
        {
            int sock = events[i].fd;
            std::uint16_t flags = events[i].flags;
            event_ = reinterpret_cast<Event*>(events[i].ctx); // current event

            if ( !event_ ) { // FIXME
                continue;
            }

            if ( flags & (M_EOF|M_ERROR) )
            {
                webserv::logger.log(webserv::Logger::ERROR, "Event socket error");
                webserv::logger.log(webserv::Logger::INFO, "Event socket disconnect");

                if ( event_->handler )
                {
                    event_->handler->event(this, flags);
                }
            }
            // если сокет есть среди слушающих, принимаем новое соединение
            else if ( listenSockets_.find(sock) != listenSockets_.end() )
            {
                webserv::logger.log(webserv::Logger::INFO, "Add new connection");
                std::map<int, struct sockaddr*>::iterator it = listenSockets_.find(sock);

                if ( event_->acceptor )
                {
                    event_->acceptor->accept(this, sock, it->second); //acceptcb call
                }
            }
            // читаем с сокета
            else if ( flags & M_READ )
            {
                webserv::logger.log(webserv::Logger::INFO, "Read in socket");

                if ( event_->reader )
                {
                    event_->reader->read(this);
                }
            }
            // пишем в сокет
            else if ( flags & M_WRITE )
            {
                webserv::logger.log(webserv::Logger::INFO, "Write in socket");

                if ( event_->writer )
                {
                    event_->writer->write(this);
                }
            }
            else {
                webserv::logger.log(webserv::Logger::INFO, "Other events");

                if ( event_->handler )
                {
                    event_->handler->event(this, flags);
                }
            }
        }
    }
}

void webserv::EventPool::addEvent(int sock, struct sockaddr *addr, std::uint16_t flags, std::int64_t time) {
    event_ = new Event(sock, addr);

    poll_.setEvent(sock, flags, event_, time);
}

void webserv::EventPool::addListener(int sock, struct sockaddr *addr, IEventAcceptor *acceptor) {
    event_ = new Event(sock, addr);
    event_->setCb(acceptor, nullptr, nullptr, nullptr);
    poll_.setEvent(sock, M_READ|M_ADD|M_CLEAR, event_); // fixme, add ev_clear
    listenSockets_.insert(std::make_pair(sock, addr));
    webserv::logger.log(webserv::Logger::INFO, "Add listen socket");
}
