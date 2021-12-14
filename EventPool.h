//
// Created by Lajuana Bespin on 10/29/21.
//

#ifndef WEBSERV_EVENTPOOL_H
#define WEBSERV_EVENTPOOL_H

#include <map>
#include "kqueue.h"

class TcpSocket;

class EventPool {
public:

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

    enum {
        M_READ      = 1 << 0,   //
        M_WRITE     = 1 << 1,   // general flags, set and returned
        M_TIMER     = 1 << 2,   //

        M_ENABLE    = 1 << 3,   //
        M_DISABLE   = 1 << 4,   //
        M_ADD       = 1 << 5,   // action flags, only set
        M_ONESHOT   = 1 << 6,   // allow mixed
        M_CLEAR     = 1 << 7,   //
        M_DELETE    = 1 << 8,   //

        M_EOF       = 1 << 9,   // returned values
        M_ERROR     = 1 << 10   //
    };

public:
    EventPool(); // throw exception
    ~EventPool();

    void start(); // throw exception
    void stop();
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

    int                 eventGetSock() const;
    struct sockaddr*    eventGetAddr() const;
    IEventAcceptor*     eventGetAcceptor() const;
    IEventReader*       eventGetReader() const;
    IEventWriter*       eventGetWriter() const;
    IEventHandler*      eventGetHandler() const;

private:
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

private:
    bool                             running_;
    Kqueue                           poll_;
    Event                            *currentEvent_;
    std::map<int, struct sockaddr*>  listenSockets_;
};

#endif //WEBSERV_EVENTPOOL_H
