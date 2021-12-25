//
// Created by Lajuana Bespin on 10/29/21.
//

#include <unistd.h>
#include <sys/event.h>
#include <deque>
#include <sys/socket.h>
#include <sys/fcntl.h>

#include <iostream>

#include "Logger.h"
#include "EventPool.h"
#include "kqueue.h"


IEventAcceptor::~IEventAcceptor() {}
IEventHandler::~IEventHandler() {}
IEventReader::~IEventReader() {}
IEventWriter::~IEventWriter() {}


EventPool::EventPool()
    : poll_(),
      listenSockets_(),
      running_(false),
      removed_(false)
{
}

EventPool::~EventPool() {

}

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
            std::uint16_t flags = events[i].flags;
            Event *event = reinterpret_cast<Event*>(events[i].ctx); // current event
            removed_ = false;
            ::printf("fd %d, flags %d\n", event->sock, flags);
            assert(event);


            if ( event->handler.get() )
            {
                event->handler->event(this, event, flags);
            }

            // читаем с сокета
            if ( !removed_ && flags & M_READ )
            {
                // если сокет есть среди слушающих, принимаем новое соединение
                std::vector<int>::iterator found = find(listenSockets_.begin(), listenSockets_.end(), event->sock);
                if ( found != listenSockets_.end() )
                {
                    webserv::logger.log(webserv::Logger::INFO, "Add new connection");

                    if ( event->acceptor.get() )
                    {
                        event->acceptor->accept(this, event); //acceptcb call
                    }
                }
                else
                {
                    webserv::logger.log(webserv::Logger::INFO, "Read in socket");
                    if ( event->reader.get() )
                    {
                        event->reader->read(this, event);
                    }
                }
            }
            // пишем в сокет
            if ( !removed_ && flags & M_WRITE )
            {
                webserv::logger.log(webserv::Logger::INFO, "Write in socket");

                if ( event->writer.get() )
                {
                    event->writer->write(this, event);
                }
            }

            if (!removed_) {
                poll_.setEvent(changeEvents_);
            }

        } // end for
    }
}

void EventPool::addEvent(Event *event, std::uint16_t flags, std::int64_t time)
{
    try {
        Kqueue::ev ev = {event->sock, flags, event, time};
        changeEvents_.push_back(ev);
    } catch (std::exception &e) {
        webserv::logger.log(webserv::Logger::ERROR, "Fail add event");
        webserv::logger.log(webserv::Logger::ERROR, e.what());
    }
}

void EventPool::removeEvent(Event *event)
{
    delete event;
    removed_ = true;
    webserv::logger.log(webserv::Logger::INFO, "Remove event");
}


void EventPool::addListener(int sock, struct sockaddr *addr, std::auto_ptr<IEventAcceptor> acceptor)
{
    try {
        Event *event = new Event(sock, addr);
        if (event == nullptr) {
            throw std::bad_alloc();
        }
        event->acceptor = acceptor; // move pointer
        changeEvents_.push_back((Kqueue::ev){sock, M_READ|M_ADD|M_CLEAR, event, 0});
        poll_.setEvent(changeEvents_);
        listenSockets_.push_back(sock);
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
Event::Event(int sock, struct sockaddr *addr)
    : sock(sock),
      addr(addr),
      acceptor(nullptr),
      reader(nullptr),
      writer(nullptr),
      handler(nullptr)
{

}

Event::~Event() {
    std::cout << "destr event\n";
    ::close(sock);
}

void Event::setCb(
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
