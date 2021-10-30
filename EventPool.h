//
// Created by Lajuana Bespin on 10/29/21.
//

#ifndef WEBSERV_EVENTPOOL_H
#define WEBSERV_EVENTPOOL_H

#include <vector>
#include <sys/event.h>
#include <deque>
#include <map>
#include "Job.h"
#include "Socket.h"

namespace webserv {
    class EventPool {
    public:
        static const int NUM_EVENTS = 1000;
    public:
        EventPool();
        virtual ~EventPool();

        void        runEventLoop();
        void        addListenSocket(webserv::Socket socket);
    private:
        std::pair<bool, const webserv::Socket&>          mFindSocket(int fd);
        int                             mKqueue;
        std::vector<webserv::Socket>    mListenSockets;
        std::deque<webserv::Job>        mWorkerJobDeque;
        std::map<int, webserv::Job>     mServerJobDeque;
    };
}


#endif //WEBSERV_EVENTPOOL_H
