//
// Created by Lajuana Bespin on 10/29/21.
//

#ifndef WEBSERV_EVENTPOOL_H
#define WEBSERV_EVENTPOOL_H

#include <map>
#include <string>
#include <TcpSocket.h>
#include "kqueue.h"

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
    virtual ~EventPool();

    void start(); // throw exception
    void stop();

    void newEvent(int socket, std::uint16_t flags, std::int64_t time = 0);
    void newListenerEvent(const TcpSocket& socket);
    void enableWriteEvent(int socket);
    void disableWriteEvent(int socket);
    void enableReadEvent(int socket);
    void disableReadEvent(int socket);
    void enableTimerEvent(int socket, int64_t time);
    void disableTimerEvent(int socket, int64_t time);


protected:
    virtual void asyncAccept(TcpSocket& socket) = 0;
    virtual void asyncRead(int socket) = 0;
    virtual void asyncWrite(int socket) = 0;
    virtual void asyncEvent(int socket, uint16_t flags) = 0;

private:
    void readHandler(int socket);
    void writeHandler(int socket);

private:
    Kqueue                      poll_;
    std::vector<TcpSocket>      listeners_;
    bool                        running_;
};

#endif //WEBSERV_EVENTPOOL_H
