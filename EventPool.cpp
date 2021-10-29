//
// Created by Lajuana Bespin on 10/29/21.
//

#include "EventPool.h"

webserv::EventPool::EventPool()
: mKqueue(0)
{
    mKqueue = kqueue();
    if (mKqueue == -1) {
        throw std::runtime_error("EventPool: kqueue()");
    }
}

webserv::EventPool::~EventPool() {

}

void webserv::EventPool::addEvent(struct kevent ev) {
    kevent(mKqueue, &ev, 1, NULL, 0, NULL);
}

const std::vector<struct kevent> webserv::EventPool::getEvents() const {
    struct kevent eventList[NUM_EVENTS];
    int nEvent = kevent(mKqueue, NULL, 0, eventList, NUM_EVENTS, NULL);
    if (nEvent == -1) {
        throw std::runtime_error("EventPool: kevent()");
    }
    return std::vector<struct kevent>(&eventList[0], &eventList[nEvent]);
}
