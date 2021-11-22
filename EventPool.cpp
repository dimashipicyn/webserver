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

namespace webserv {
    class EventPool {
    public:
        struct Event;
        typedef void (*acceptCB)(EventPool *evt, int sd, struct sockaddr *addr);
        typedef void (*eventCB)(EventPool *evt, struct Event *event, std::uint16_t flags, std::uintptr_t ctx);
        typedef void (*readCB)(EventPool *evt, struct Event *event, std::uintptr_t ctx);
        typedef void (*writeCB)(EventPool *evt, struct Event *event, std::uintptr_t ctx);

        enum type {
            READ  = -1,
            WRITE = -2,
            TIMER = -7
        };

        struct Event {
            explicit Event(int sd);
            ~Event();
            void setCb(readCB rcb, writeCB wcb, eventCB ecb, std::uintptr_t ctx);
            int getSd();
            int             sd;
            std::uintptr_t  usrCtx;
            readCB          readCb;
            writeCB         writeCb;
            eventCB         eventCb;
        };

    public:
        EventPool();
        virtual ~EventPool();

        void eventLoop();
        void addListenSocket(struct sockaddr *addr, acceptCB acceptCb);
        void addEvent(struct Event *event);
        void eventEnable(struct Event *event, std::int16_t flags);
        void eventDisable(struct Event *event, std::int16_t flags);

    private:
        int                                                     mKqueue;
        std::map<int, std::pair<struct sockaddr *, acceptCB> >  mListenSockets;
    };
}

webserv::EventPool::Event::Event(int sd) : sd(sd) {}

webserv::EventPool::Event::~Event() {
    ::close(sd);
}

void webserv::EventPool::Event::setCb(webserv::EventPool::readCB rcb, webserv::EventPool::writeCB wcb,
                                      webserv::EventPool::eventCB ecb, std::uintptr_t ctx)
{
        usrCtx = ctx;
        readCb = rcb;
        eventCb = ecb;
        writeCb = wcb;
}

int webserv::EventPool::Event::getSd() {
    return sd;
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

void webserv::EventPool::addEvent(struct Event *event) {
    struct kevent chEvent = {};
    EV_SET(&chEvent, event->sd, 0, 0, 0, 0, event);
    kevent(mKqueue, &chEvent, 1, NULL, 0, NULL);
}

void webserv::EventPool::eventEnable(struct Event *event, std::int16_t flags) {
    struct kevent chEvent = {};
    EV_SET(&chEvent, event->sd, flags, EV_ADD|EV_ENABLE, 0, 0, event);
    kevent(mKqueue, &chEvent, 1, NULL, 0, NULL);
}

void webserv::EventPool::eventDisable(struct Event *event, std::int16_t flags) {
    struct kevent chEvent = {};
    EV_SET(&chEvent, event->sd, flags, EV_DISABLE, 0, 0, event);
    kevent(mKqueue, &chEvent, 1, NULL, 0, NULL);
}

void webserv::EventPool::eventLoop() {
    struct kevent   chEvent;
    const int       NUM_EVENTS = 1024;
    struct kevent   eventList[NUM_EVENTS];
    const int       BUFFER_SIZE = 1024;
    char            buffer[BUFFER_SIZE + 1];

    webserv::logger.log(webserv::Logger::INFO, "Run event loop");
    while (true) {
        int nEvent = kevent(mKqueue, NULL, 0, eventList, NUM_EVENTS, NULL);

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
                event->readCb(this, event, event->usrCtx);
            }
            // пишем в сокет
            else if (eventList[i].filter == EVFILT_WRITE)
            {
                webserv::logger.log(webserv::Logger::INFO, "Write in socket");
                event->writeCb(this, event, event->usrCtx);
            }
            else if (eventList[i].filter == EVFILT_TIMER)
            {
                webserv::logger.log(webserv::Logger::INFO, "Query timeout!");
            }
        }
    }
}

static int createListener(struct sockaddr *addr) {
    int sd;
    // Creating socket file descriptor
    // SOCK_STREAM потоковый сокет
    // IPPROTO_TCP протокол TCP
    if ( (sd = socket(AF_INET, SOCK_STREAM, 0)) == 0 ) {
        throw std::runtime_error("Listener: create socket failed");
    }

    // неблокирующий сокет
    if ( fcntl(sd, F_SETFL, O_NONBLOCK) == -1 ) {
        ::close(sd);
        throw std::runtime_error("Listener: fcntl failed");
    }

    // опции сокета, для переиспользования локальных адресов
    if ( setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (int[]){1}, sizeof(int)) ) {
        ::close(sd);
        throw std::runtime_error("Listener: setsockopt failed");
    }

    struct sockaddr_in  m_address;
    m_address.sin_family = AF_INET; // domain AF_INET для ipv4, AF_INET6 для ipv6, AF_UNIX для локальных сокетов
    m_address.sin_addr.s_addr = inet_addr("127.0.0.1"); // адрес host, format "127.0.0.1"
    m_address.sin_port = htons(1234); // host-to-network short

    socklen_t addrLen = sizeof(m_address);
    // связывает сокет с конкретным адресом
    if (bind(sd, (struct sockaddr *)&m_address, addrLen) < 0 ) {
        ::close(sd);
        perror("listener");
        throw std::runtime_error("Listener: bind socket failed");
    }

    // готовит сокет к принятию входящих соединений
    if (listen(sd, SOMAXCONN) < 0 ) {
        ::close(sd);
        throw std::runtime_error("Listener: listen socket failed");
    }
    return sd;
}

void webserv::EventPool::addListenSocket(struct sockaddr *addr, acceptCB acceptCb) {
    struct kevent chEvent = {0};
    int sd = -1;
    try {
        sd = createListener(addr);
    }
    catch (std::exception &e) {
        throw ;
    }
    EV_SET(&chEvent, sd, EVFILT_READ, EV_ADD | EV_CLEAR, 0, 0, NULL);
    int res = kevent(mKqueue, &chEvent, 1, NULL, 0, NULL);
    if (res == -1) {
        throw std::runtime_error("register socket descriptor for kqueue() fail");
    }
    mListenSockets.insert(std::make_pair(sd, std::make_pair(addr, acceptCb)));
    webserv::logger.log(webserv::Logger::INFO, "Add listen socket");
}
