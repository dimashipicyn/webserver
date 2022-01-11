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


/*********** callbacks ****************/
struct ISessionCallback {
public:
    virtual ~ISessionCallback();
    virtual void asyncAccept(Session& session);
    virtual void asyncRead(Session& session);
    virtual void asyncWrite(Session& session);
    virtual void asyncEvent(Session& session, std::uint16_t flags);
};
/********** callbacks end **********/

struct Session {
    Session();
    ~Session();

    std::auto_ptr<TcpSocket>        socket;
    std::auto_ptr<ISessionCallback> cb;
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

    typedef std::map<int, Session> sessionMap;

public:
    EventPool(); // throw exception
    ~EventPool();

    void start(); // throw exception
    void stop();

    void newSession(Session& session, std::uint16_t flags, std::int64_t time = 0);
    void newListener(Session& session)
    ;
private:
    Kqueue                      poll_;
    std::vector<int>            listeners_;
    sessionMap                  sessionMap_;
    bool                        running_;
};

#endif //WEBSERV_EVENTPOOL_H
