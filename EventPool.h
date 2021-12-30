//
// Created by Lajuana Bespin on 10/29/21.
//

#ifndef WEBSERV_EVENTPOOL_H
#define WEBSERV_EVENTPOOL_H

#include <map>
#include "kqueue.h"

class TcpSocket;
class EventPool;
struct Event;


/*********** callbacks ****************/
class IEventAcceptor {
public:
    virtual ~IEventAcceptor();
    virtual void accept(EventPool *evPool, Event *event) = 0;
};

class IEventReader {
public:
    virtual~IEventReader();
    virtual void read(EventPool *evPool, Event *event) = 0;
};

class IEventWriter {
public:
    virtual ~IEventWriter();
    virtual void write(EventPool *evPool, Event *event) = 0;
};

class IEventHandler {
public:
    virtual ~IEventHandler();
    virtual void event(EventPool *evPool, Event *event, std::uint16_t flags) = 0;
};
/********** callbacks end **********/

struct Event {
    Event(int sock, struct sockaddr *addr);
    ~Event();

    void            setCb(
            std::auto_ptr<IEventAcceptor> acceptor,
            std::auto_ptr<IEventReader> reader,
            std::auto_ptr<IEventWriter> writer,
            std::auto_ptr<IEventHandler> handler
            );

    int             sock;
    struct sockaddr *addr;
    std::auto_ptr<IEventAcceptor> acceptor;
    std::auto_ptr<IEventReader> reader;
    std::auto_ptr<IEventWriter> writer;
    std::auto_ptr<IEventHandler> handler;
};

class EventPool {
public:
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
    void addListener(int sock, struct sockaddr *addr, std::auto_ptr<IEventAcceptor> acceptor);
    void addEvent(Event *event, std::uint16_t flags, std::int64_t time = 0);
    void removeEvent(Event *event);

private:
    Kqueue                      poll_;
    std::vector<int>            listenSockets_;
    std::vector<Kqueue::ev>     changeEvents_;
    bool                        running_;
    bool                        removed_;
};

#endif //WEBSERV_EVENTPOOL_H
