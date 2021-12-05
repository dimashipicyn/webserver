//
// Created by Lajuana Bespin on 10/29/21.
//

#ifndef WEBSERV_EVENTPOOL_H
#define WEBSERV_EVENTPOOL_H

#include <map>
#include "kqueue.h"

namespace webserv {
    class EventPool;

    class IEventAcceptor {
    public:
        virtual void accept(EventPool *evPool, int sock, struct sockaddr *addr) = 0;
    };

    class IEventReader {
    public:
        virtual void read(EventPool *evPool) = 0;
    };

    class IEventWriter {
    public:
        virtual void write(EventPool *evPool) = 0;
    };

    class IEventHandler {
    public:
        virtual void event(EventPool *evPool, std::uint16_t flags) = 0;
    };

    class EventPool {
    public:
        enum {
            M_READ = 1,
            M_WRITE = 2,
            M_TIMER = 4,
            M_EOF = 8, // return value
            M_ERROR = 16, // return value
            M_ENABLE = 32,
            M_DISABLE = 64,
            M_ADD = 128,
            M_ONESHOT = 256,
            M_CLEAR = 512
        };
        struct Event {
            Event(int sock, struct sockaddr *addr);
            ~Event();

            void            setCb(
                IEventAcceptor *acceptor,
                IEventReader *reader,
                IEventWriter *writer,
                IEventHandler *handler
            );

            int             sock;
            struct sockaddr *addr;
            IEventAcceptor  *acceptor;
            IEventReader    *reader;
            IEventWriter    *writer;
            IEventHandler   *handler;
        };

    public:
        EventPool();
        ~EventPool();

        void eventLoop();
        void addListener(int sock, struct sockaddr *addr, IEventAcceptor *acceptor);
        void addEvent(int sock, struct sockaddr *addr, std::uint16_t flags, std::int64_t time = 0);

        // event methods
        void eventSetFlags(std::uint16_t flags, std::int64_t time = 0);
        void eventSetAccepter(IEventAcceptor *acceptor);
        void eventSetReader(IEventReader *reader);
        void eventSetWriter(IEventWriter *writer);
        void eventSetHandler(IEventHandler *handler);
        void eventSetCb(
                IEventAcceptor *acceptor,
                IEventReader *reader,
                IEventWriter *writer,
                IEventHandler *handler
                );

        int                 eventGetSock();
        struct sockaddr*    eventGetAddr();
        IEventAcceptor*     eventGetAcceptor();
        IEventReader*       eventGetReader();
        IEventWriter*       eventGetWriter();
        IEventHandler*      eventGetHandler();

    private:
        Kqueue                           poll_;
        Event                            *event_;
        std::map<int, struct sockaddr*>  listenSockets_;
    };
}

#endif //WEBSERV_EVENTPOOL_H
