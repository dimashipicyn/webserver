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

webserv::EventPool::Event::Event(int sock, struct sockaddr *addr)
        : m_sock(sock), m_addr(addr), m_ctx(0), m_readCb(nullptr), m_writeCb(nullptr), m_eventCb(nullptr) {

}

webserv::EventPool::Event::~Event() {
    ::close(m_sock);
}

void webserv::EventPool::Event::setCb(readCB rcb, writeCB wcb, eventCB ecb, std::uintptr_t ctx) {
    m_ctx = ctx;
    m_readCb = rcb;
    m_eventCb = ecb;
    m_writeCb = wcb;
}

int webserv::EventPool::Event::getSock() {
    return m_sock;
}

struct sockaddr *webserv::EventPool::Event::getAddr() {
    return m_addr;
}

std::uintptr_t webserv::EventPool::Event::getCtx() {
    return m_ctx;
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

void webserv::EventPool::eventAdd(struct Event *event) {
    struct kevent chEvent = {};
    EV_SET(&chEvent, event->getSock(), 0, 0, 0, 0, event);
    kevent(mKqueue, &chEvent, 1, NULL, 0, nullptr);
}

void webserv::EventPool::eventEnable(struct Event *event, std::int16_t flags) {
    struct kevent chEvent = {};
    EV_SET(&chEvent, event->getSock(), flags, EV_ADD|EV_ENABLE, 0, 0, event);
    kevent(mKqueue, &chEvent, 1, nullptr, 0, nullptr);
}

void webserv::EventPool::eventDisable(struct Event *event, std::int16_t flags) {
    struct kevent chEvent = {};
    EV_SET(&chEvent, event->getSock(), flags, EV_DISABLE, 0, 0, event);
    kevent(mKqueue, &chEvent, 1, nullptr, 0, nullptr);
}

void webserv::EventPool::eventLoop() {
    struct kevent   chEvent;
    const int       NUM_EVENTS = 1024;
    struct kevent   eventList[NUM_EVENTS];
    const int       BUFFER_SIZE = 1024;
    char            buffer[BUFFER_SIZE + 1];

    webserv::logger.log(webserv::Logger::INFO, "Run event loop");
    while (true) {
        int nEvent = kevent(mKqueue, nullptr, 0, eventList, NUM_EVENTS, nullptr);

        if (nEvent == -1) {
            throw std::runtime_error("EventPool: kevent()");
        }

        for (int i = 0; i < nEvent; ++i)
        {
            int sd = static_cast<int>(eventList[i].ident);
            struct Event *event = reinterpret_cast<Event*>(eventList[i].udata);

            if (eventList[i].flags & EV_EOF || eventList[i].flags & EV_ERROR)
            {
                webserv::logger.log(webserv::Logger::ERROR, "Event socket error");
                webserv::logger.log(webserv::Logger::INFO, "Event socket disconnect");
                std::cout << eventList[i].flags << std::endl;
                //event->eventCb(event, eventList[i].flags, event->usrCtx);
            }
            // если сокет есть среди слушающих, принимаем новое соединение
            else if (mListenSockets.find(sd) != mListenSockets.end() )
            {
                webserv::logger.log(webserv::Logger::INFO, "Add new connection");
                std::map<int, std::pair<struct sockaddr*, acceptCB> >::iterator it = mListenSockets.find(sd);
                it->second.second(this, sd, it->second.first); // acceptCb
            }
            // читаем с сокета
            else if (eventList[i].filter == EVFILT_READ)
            {
                webserv::logger.log(webserv::Logger::INFO, "Read in socket");
                event->m_readCb(this, event, event->getCtx());
            }
            // пишем в сокет
            else if (eventList[i].filter == EVFILT_WRITE)
            {
                webserv::logger.log(webserv::Logger::INFO, "Write in socket");
                event->m_writeCb(this, event, event->getCtx());
            }
            else if (eventList[i].filter == EVFILT_TIMER)
            {
                webserv::logger.log(webserv::Logger::INFO, "Query timeout!");
            }
        }
    }
}

void webserv::EventPool::addListener(int sock, struct sockaddr *addr, acceptCB acceptCb) {
    struct kevent chEvent = {0};

    EV_SET(&chEvent, sock, EVFILT_READ, EV_ADD | EV_CLEAR, 0, 0, NULL);
    int res = kevent(mKqueue, &chEvent, 1, nullptr, 0, nullptr);
    if ( res == -1 ) {
        throw std::runtime_error("register socket descriptor for kqueue() fail");
    }
    mListenSockets.insert(std::make_pair(sock, std::make_pair(addr, acceptCb)));
    webserv::logger.log(webserv::Logger::INFO, "Add listen socket");
}
