//
// Created by Lajuana Bespin on 10/29/21.
//

#include <map>

#ifndef WEBSERV_EVENTPOOL_H
#define WEBSERV_EVENTPOOL_H

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
            READ  = -1,
            WRITE = -2,
            TIMER = -7
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
        void addEvent(int sock, struct sockaddr *addr);

        // event methods
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

        void eventEnable(std::int16_t flags, std::int32_t time = 0);
        void eventDisable(std::int16_t flags);

    private:
        int                              mKqueue;
        Event                            *event_;
        std::map<int, struct sockaddr*>  mListenSockets;
    };
}

#endif //WEBSERV_EVENTPOOL_H
