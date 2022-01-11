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

void ISessionCallback::asyncAccept(Session& session) {(void)session;}
void ISessionCallback::asyncRead(Session& session) {(void)session;}
void ISessionCallback::asyncWrite(Session &session) {(void)session;}
void ISessionCallback::asyncEvent(Session& session, std::uint16_t flags) {(void)session;(void)flags;}



EventPool::EventPool()
    : poll_(),
      running_(false)
{
}

EventPool::~EventPool() {

}

void EventPool::start() {
    int sizeEvents = 1024;
    std::vector<Kqueue::ev> events(sizeEvents);

    LOG_INFO("Run event loop\n");

    running_ = true;
    while (running_) {
        int n = poll_.getEvents(events); // throw
        for (int i = 0; i < n; ++i)
        {
            uint16_t flags = events[i].flags;
            int32_t socket = events[i].fd;

            sessionMap::iterator foundSession = sessionMap_.find(socket);
            if (foundSession == sessionMap_.end()) {
                ::close(socket);
                continue;
            }

            foundSession->second.cb->asyncEvent(foundSession->second, flags);

            // читаем с сокета
            if ( flags & M_READ )
            {
                // если сокет есть среди слушающих, принимаем новое соединение
                std::vector<int>::iterator foundListener = find(listeners_.begin(), listeners_.end(), socket);
                if ( foundListener != listeners_.end() )
                {
                    LOG_DEBUG("Create new connection\n");

                    if (foundSession->second.isClosed == false) {
                        foundSession->second.cb->asyncAccept(foundSession->second);
                    }
                }
                else
                {
                    LOG_DEBUG("Reading in fd: %d\n", socket);

                    if (foundSession->second.isClosed == false) {
                        foundSession->second.cb->asyncRead(foundSession->second);
                    }
                }
            }
            // пишем в сокет
            if ( flags & M_WRITE )
            {
                LOG_DEBUG("Write in fd: %d\n", socket);

                if (foundSession->second.isClosed == false) {
                    foundSession->second.cb->asyncWrite(foundSession->second);
                }
            }

            if (foundSession->second.isClosed) {
                sessionMap_.erase(foundSession->first);
            }
        } // end for
    }
}

void EventPool::newSession(Session& session, std::uint16_t flags, std::int64_t time)
{
    try {
        poll_.setEvent(session.socket->getSock(), flags, nullptr, time);
        sessionMap_[session.socket->getSock()] = session;
    } catch (std::exception &e) {
        LOG_DEBUG("Failed addEvent: %s\n", e.what());
    }
}

void EventPool::newListener(Session& session)
{
    try {
        poll_.setEvent(session.socket->getSock(), M_READ|M_ADD|M_CLEAR, nullptr, 0);
        listeners_.push_back(session.socket->getSock());
        LOG_DEBUG("Add listen fd: %d\n", session.socket->getSock());
    } catch (std::exception &e) {
        LOG_DEBUG("addListener fail. %s\n", e.what());
    }
}

void EventPool::stop() {
    running_ = false;
}
