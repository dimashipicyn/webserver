//
// Created by Lajuana Bespin on 10/29/21.
//

#ifndef WEBSERV_EVENTPOOL_H
#define WEBSERV_EVENTPOOL_H

#include <vector>
#include <sys/event.h>

namespace webserv {
    class EventPool {
    public:
        static const int NUM_EVENTS = 1000;
    public:
        EventPool();
        virtual ~EventPool();

        void                                addEvent(struct kevent ev);
        const std::vector<struct kevent>    getEvents() const;
    private:
        int                 mKqueue;
    };
}


#endif //WEBSERV_EVENTPOOL_H
