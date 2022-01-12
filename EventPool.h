//
// Created by Lajuana Bespin on 10/29/21.
//

#ifndef WEBSERV_EVENTPOOL_H
#define WEBSERV_EVENTPOOL_H

#include <map>
#include "kqueue.h"

class TcpSocket;
class EventPool;
struct Session;


struct ISession {
public:
    ISession();
    virtual ~ISession();

    void close();

    // callback
    virtual void asyncAccept(EventPool& evPool);
    virtual void asyncRead(char *buffer, int64_t bytes);
    virtual void asyncWrite(char *buffer, int64_t bytes);
    virtual void asyncEvent(std::uint16_t flags);

    std::auto_ptr<TcpSocket>        socket;
    bool                            isClosed;
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

    typedef std::map<int, ISession> sessionMap;

public:
    EventPool(); // throw exception
    ~EventPool();

    void start(); // throw exception
    void stop();

    void newSession(ISession& session, std::uint16_t flags, std::int64_t time = 0);
    void newListenSession(ISession& session)
    ;
private:
    enum {
        readBufferSize = (1 << 16),
        writeBufferSize = (1 << 16)
    };
    Kqueue                      poll_;
    std::vector<int>            listeners_;
    sessionMap                  sessionMap_;
    char                        readBuffer_[readBufferSize];
    char                        writeBuffer_[writeBufferSize];
    bool                        running_;
};

#endif //WEBSERV_EVENTPOOL_H
