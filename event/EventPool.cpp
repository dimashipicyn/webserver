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
#include "TcpSocket.h"


EventPool::EventPool()
    : poll_()
    , listeners_()
    , running_(false)
{
}

EventPool::~EventPool() {

}

void EventPool::readHandler(int socket)
{
    // если сокет есть среди слушающих, принимаем новое соединение
    std::vector<TcpSocket>::iterator foundListener = find(listeners_.begin(), listeners_.end(), socket);
    if (foundListener != listeners_.end())
    {
        LOG_DEBUG("Create new connection\n");
        asyncAccept(*foundListener);
    }
    else
    {
        LOG_DEBUG("Reading in fd: %d\n", socket);
        asyncRead(socket);
    }
}

void EventPool::writeHandler(int socket)
{
    asyncWrite(socket);
}

void EventPool::start() {
    int sizeEvents = 8192;
    std::vector<Kqueue::ev> events(sizeEvents);

    LOG_INFO("Run event loop\n");

    running_ = true;
    while (running_) {
        int n = poll_.getEvents(events); // throw
        for (int i = 0; i < n; ++i)
        {
            uint16_t flags = events[i].flags;
            int32_t socket = events[i].fd;

            asyncEvent(socket, flags);

            // читаем с сокета
            if (flags & M_READ)
            {
                readHandler(socket);
            }
            // пишем в сокет
            if (flags & M_WRITE)
            {
                LOG_DEBUG("Write in fd: %d\n", socket);
                writeHandler(socket);
            }
        } // end for
    }
}

void EventPool::newEvent(int socket, std::uint16_t flags, std::int64_t time)
{
    try {
        poll_.setEvent(socket, flags, nullptr, time);
    } catch (std::exception &e) {
        LOG_DEBUG("Failed addEvent: %s\n", e.what());
    }
}

void EventPool::newListenerEvent(const TcpSocket& socket)
{
    try {
        poll_.setEvent(socket.getSock(), M_READ|M_ADD|M_CLEAR, nullptr, 0);
        listeners_.push_back(socket);
        LOG_DEBUG("Add listen fd: %d\n", socket.getSock());
    } catch (std::exception &e) {
        LOG_DEBUG("addListener fail. %s\n", e.what());
    }
}

void EventPool::enableReadEvent(int socket)
{
    newEvent(socket, M_READ|M_ADD|M_ENABLE);
}

void EventPool::disableReadEvent(int socket)
{
    newEvent(socket, M_READ|M_DISABLE);
}

void EventPool::enableWriteEvent(int socket)
{
    newEvent(socket, M_WRITE|M_ADD|M_ENABLE);
}

void EventPool::disableWriteEvent(int socket)
{
    newEvent(socket, M_WRITE|M_DISABLE);
}

void EventPool::enableTimerEvent(int socket, int64_t time)
{
    newEvent(socket, M_TIMER|M_ADD|M_ENABLE, time);
}

void EventPool::disableTimerEvent(int socket, int64_t time)
{
    newEvent(socket, M_TIMER|M_DISABLE, time);
}

void EventPool::stop() {
    running_ = false;
}
