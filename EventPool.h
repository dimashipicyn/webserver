//
// Created by Lajuana Bespin on 10/29/21.
//

#ifndef WEBSERV_EVENTPOOL_H
#define WEBSERV_EVENTPOOL_H

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
            explicit    Event(int sd);
                        ~Event();
            void        setCb(readCB rcb, writeCB wcb, eventCB ecb, std::uintptr_t ctx);
            int         getSd();
        };

    public:
        EventPool();
        virtual ~EventPool();

        void eventLoop();
        void addListenSocket(struct sockaddr *addr, acceptCB acceptCb);
        void addEvent(struct Event *event);
        void eventEnable(struct Event *event, std::int16_t flags);
        void eventDisable(struct Event *event, std::int16_t flags);
    };
}

#endif //WEBSERV_EVENTPOOL_H