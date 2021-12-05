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
: mKqueue(0), mListenSockets()
{
    mKqueue = kqueue();
    if (mKqueue == -1) {
        throw std::runtime_error("EventPool: kqueue()");
    }
}

webserv::EventPool::~EventPool() {

}

void webserv::EventPool::eventSetCb(
        webserv::IEventAcceptor *acc,
        webserv::IEventReader *reader,
        webserv::IEventWriter *writer,
        webserv::IEventHandler *handler
        ) {
    struct kevent chEvent = {};
    event_->setCb(acc, reader, writer, handler);
    EV_SET(&chEvent, event_->sock, 0, 0, 0, 0, event_);
    kevent(mKqueue, &chEvent, 1, NULL, 0, nullptr);
}

void webserv::EventPool::eventEnable(std::int16_t flags, std::int32_t time) {
    struct kevent chEvent = {};
    assert(event_);
    EV_SET(&chEvent, event_->sock, flags, EV_ADD|EV_ENABLE, 0, time, event_);
    kevent(mKqueue, &chEvent, 1, nullptr, 0, nullptr);
}

void webserv::EventPool::eventDisable(std::int16_t flags) {
    struct kevent chEvent = {};
    assert(event_);
    EV_SET(&chEvent, event_->sock, flags, EV_DISABLE, 0, 0, event_);
    kevent(mKqueue, &chEvent, 1, nullptr, 0, nullptr);
}

int webserv::EventPool::eventGetSock() {
    return event_->sock;
}


void webserv::EventPool::eventLoop() {
    const int       NUM_EVENTS = 1024;
    struct kevent   eventList[NUM_EVENTS];

    webserv::logger.log(webserv::Logger::INFO, "Run event loop");
    while (true) {
        int nEvent = kevent(mKqueue, nullptr, 0, eventList, NUM_EVENTS, nullptr);

        if (nEvent == -1) {
            throw std::runtime_error("EventPool: kevent()");
        }

        for (int i = 0; i < nEvent; ++i)
        {
            int sock = static_cast<int>(eventList[i].ident);
            event_ = reinterpret_cast<Event*>(eventList[i].udata); // current event

            if ( !event_ ) { // FIXME
                continue;
            }

            if ( eventList[i].flags & (EV_EOF|EV_ERROR) )
            {
                webserv::logger.log(webserv::Logger::ERROR, "Event socket error");
                webserv::logger.log(webserv::Logger::INFO, "Event socket disconnect");

                if ( event_->handler )
                {
                    event_->handler->event(this, eventList[i].flags);
                }
            }
            // если сокет есть среди слушающих, принимаем новое соединение
            else if ( mListenSockets.find(sock) != mListenSockets.end() )
            {
                webserv::logger.log(webserv::Logger::INFO, "Add new connection");
                std::map<int, struct sockaddr*>::iterator it = mListenSockets.find(sock);

                if ( event_->acceptor )
                {
                    event_->acceptor->accept(this, sock, it->second); //acceptcb call
                }
            }
            // читаем с сокета
            else if ( eventList[i].filter == EVFILT_READ )
            {
                webserv::logger.log(webserv::Logger::INFO, "Read in socket");

                if ( event_->reader )
                {
                    event_->reader->read(this);
                }
            }
            // пишем в сокет
            else if ( eventList[i].filter == EVFILT_WRITE )
            {
                webserv::logger.log(webserv::Logger::INFO, "Write in socket");

                if ( event_->writer )
                {
                    event_->writer->write(this);
                }
            }
            else if ( eventList[i].filter == EVFILT_TIMER )
            {
                webserv::logger.log(webserv::Logger::INFO, "Query timeout!");
            }
        }
    }
}

void webserv::EventPool::addEvent(int sock, struct sockaddr *addr) {
    event_ = new Event(sock, addr);

    struct kevent chEvent = {};
    EV_SET(&chEvent, event_->sock, 0, 0, 0, 0, event_);
    kevent(mKqueue, &chEvent, 1, NULL, 0, nullptr);
}

void webserv::EventPool::addListener(int sock, struct sockaddr *addr, IEventAcceptor *acceptor) {
    struct kevent chEvent = {};

    Event *event = new Event(sock, addr);
    event->setCb(acceptor, nullptr, nullptr, nullptr);
    EV_SET(&chEvent, sock, EVFILT_READ, EV_ADD | EV_CLEAR, 0, 0, reinterpret_cast<void*>(event));
    int res = kevent(mKqueue, &chEvent, 1, nullptr, 0, nullptr);
    if ( res == -1 ) {
        throw std::runtime_error("register socket descriptor for kqueue() fail");
    }
    mListenSockets.insert(std::make_pair(sock, addr));
    webserv::logger.log(webserv::Logger::INFO, "Add listen socket");
}
