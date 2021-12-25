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
#include "TcpSocket.h"

IEventAcceptor::~IEventAcceptor() {}
IEventHandler::~IEventHandler() {}
IEventReader::~IEventReader() {}
IEventWriter::~IEventWriter() {}


EventPool::EventPool()
    : poll_(),
      currentEvent_(nullptr),
      listenSockets_()
{
}

EventPool::~EventPool() {

}

void EventPool::eventSetCb(
        std::auto_ptr<IEventAcceptor> acceptor,
        std::auto_ptr<IEventReader> reader,
        std::auto_ptr<IEventWriter> writer,
        std::auto_ptr<IEventHandler> handler
        ) {
    currentEvent_->setCb(acceptor, reader, writer, handler);
}

void EventPool::eventSetFlags(std::uint16_t flags, std::int64_t time) {
    Kqueue::ev event = {
        .fd = currentEvent_->sock,
        .flags = flags,
        .ctx = currentEvent_,
        .data = time
    };
    changeEvents_.push_back(event);
}

/*
void EventPool::eventSetAccepter(IEventAcceptor *acceptor) {
    currentEvent_->acceptor = acceptor;
}

void EventPool::eventSetReader(IEventReader *reader) {
    currentEvent_->reader = reader;
}
void EventPool::eventSetWriter(IEventWriter *writer) {
    currentEvent_->writer = writer;
}
void EventPool::eventSetHandler(IEventHandler *handler) {
    currentEvent_->handler = handler;
}
*/
int EventPool::eventGetSock() const {
    return currentEvent_->sock;
}

struct sockaddr* EventPool::eventGetAddr() const {
    return currentEvent_->addr;
}
/*
EventPool::IEventAcceptor* EventPool::eventGetAcceptor() const {
    return currentEvent_->acceptor;
}

EventPool::IEventReader* EventPool::eventGetReader() const {
    return currentEvent_->reader;
}
EventPool::IEventWriter* EventPool::eventGetWriter() const {
    return currentEvent_->writer;
}
EventPool::IEventHandler* EventPool::eventGetHandler() const {
    return currentEvent_->handler;
}
*/
void EventPool::start() {
    int sizeEvents = 1024;
    std::vector<Kqueue::ev> events(sizeEvents);

    webserv::logger.log(webserv::Logger::INFO, "Run event loop");

    running_ = true;
    while (running_) {
        int n = poll_.getEvents(events); // throw

        for (int i = 0; i < n; ++i)
        {
            changeEvents_.clear();

            int sock = events[i].fd;
            std::uint16_t flags = events[i].flags;
            currentEvent_ = reinterpret_cast<Event*>(events[i].ctx); // current event

            ::printf("fd %d, flags %d\n", sock, flags);
            assert(currentEvent_);


            if ( currentEvent_->handler.get() )
            {
                currentEvent_->handler->event(this, flags);
            }

            // читаем с сокета
            if ( flags & M_READ )
            {
                // если сокет есть среди слушающих, принимаем новое соединение
                if ( listenSockets_.find(sock) != listenSockets_.end() )
                {
                    webserv::logger.log(webserv::Logger::INFO, "Add new connection");

                    std::map<int, struct sockaddr*>::iterator it = listenSockets_.find(sock);
                    if ( currentEvent_->acceptor.get() )
                    {
                        currentEvent_->acceptor->accept(this, sock, it->second); //acceptcb call
                    }
                }
                else
                {
                    webserv::logger.log(webserv::Logger::INFO, "Read in socket");
                    if ( currentEvent_->reader.get() )
                    {
                        currentEvent_->reader->read(this);
                    }
                }
            }
            // пишем в сокет
            if ( flags & M_WRITE )
            {
                webserv::logger.log(webserv::Logger::INFO, "Write in socket");

                if ( currentEvent_->writer.get() )
                {
                    currentEvent_->writer->write(this);
                }
            }
            // remove event
            if (removeCurrentEvent_) {
                delete(currentEvent_);
                currentEvent_ = nullptr;
                removeCurrentEvent_ = false;
            }
            else {
                poll_.setEvent(changeEvents_);
            }
        } // end for
    }
}

void EventPool::addEvent(int sock, struct sockaddr *addr, std::uint16_t flags, std::int64_t time)
{
    try {
        currentEvent_ = new Event(sock, addr);
        if (!currentEvent_) {
            throw std::bad_alloc();
        }
        eventSetFlags(flags, time);
    } catch (std::exception &e) {
        webserv::logger.log(webserv::Logger::ERROR, "Fail add event");
        webserv::logger.log(webserv::Logger::ERROR, e.what());
    }
}

void EventPool::removeEvent()
{
    removeCurrentEvent_ = true;
    webserv::logger.log(webserv::Logger::INFO, "Remove event");
}


void EventPool::addListener(int sock, struct sockaddr *addr, std::auto_ptr<IEventAcceptor> acceptor)
{
    try {
        Event *event = new Event(sock, addr);
        if (!event) {
            throw std::bad_alloc();
        }
        event->acceptor = acceptor; // move pointer
        std::vector<Kqueue::ev> ch;
        ch.push_back((Kqueue::ev){sock, M_READ|M_ADD|M_CLEAR, event, 0});
        poll_.setEvent(ch);
        listenSockets_.insert(std::make_pair(sock, addr));
        webserv::logger.log(webserv::Logger::INFO, "Add listen socket");
    } catch (std::exception &e) {
        webserv::logger.log(webserv::Logger::ERROR, "Fail add listen socket");
        webserv::logger.log(webserv::Logger::ERROR, e.what());
    }
}

void EventPool::stop() {
    running_ = false;
}

//////////////////////////////////////
/////////////// Event ////////////////
//////////////////////////////////////
EventPool::Event::Event(int sock, struct sockaddr *addr)
    : sock(sock),
      addr(addr),
      acceptor(nullptr),
      reader(nullptr),
      writer(nullptr),
      handler(nullptr)
{

}

EventPool::Event::~Event() {
    std::cout << "destr event\n";
    ::close(sock);
}

void EventPool::Event::setCb(
        std::auto_ptr<IEventAcceptor> acc,
        std::auto_ptr<IEventReader> re,
        std::auto_ptr<IEventWriter> wr,
        std::auto_ptr<IEventHandler> h)
{
    acceptor = acc;
    reader = re;
    writer = wr;
    handler = h;
}
