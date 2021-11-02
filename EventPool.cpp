//
// Created by Lajuana Bespin on 10/29/21.
//

#include <unistd.h>
#include "EventPool.h"
#include "Logger.h"

webserv::EventPool::EventPool()
: mKqueue(0), mListenSockets()
{
    mKqueue = kqueue();
    if (mKqueue == -1) {
        throw std::runtime_error("EventPool: kqueue()");
    }
}

webserv::EventPool::~EventPool() {

}

void webserv::EventPool::eventLoop() {
    struct kevent   chEvent;
    struct kevent   eventList[NUM_EVENTS];
    const int       BUFFER_SIZE = 1024;
    char            buffer[BUFFER_SIZE + 1];

    webserv::logger.log(webserv::Logger::INFO, "Run event loop");
    while (true) {
        int nEvent = kevent(mKqueue, NULL, 0, eventList, NUM_EVENTS, NULL);

        if (nEvent == -1) {
            throw std::runtime_error("EventPool: kevent()");
        }

        for (int i = 0; i < nEvent; ++i) {
            if (eventList[i].flags & EV_ERROR) {
                webserv::logger.log(webserv::Logger::ERROR, "Event socket error");
                continue;
            }

            int fd = (int)eventList[i].ident;

            // если соединение закрыто
            if (eventList[i].flags & EV_EOF) {
                webserv::logger.log(webserv::Logger::INFO, "Event socket disconnect");
                eventList[i].udata = 0;
                close(fd); // Socket is automatically removed from the kq by the kernel.
            }

            // если сокет есть среди слушающих, принимаем новое соединение
            else if ( mListenSockets.find(fd) != mListenSockets.end() )
            {
                int newConnect = mListenSockets.find(fd)->second.acceptConnection();

                Job *job = new Job; // новая задача
                // добавляем в ядро новый дескриптор
                EV_SET(&chEvent, newConnect, EVFILT_READ, EV_ADD, 0, 0, job);
                kevent(mKqueue, &chEvent, 1, NULL, 0, NULL);

                webserv::logger.log(webserv::Logger::INFO, "Add new connection");
            }
            // читаем с сокета
            else if (eventList[i].filter == EVFILT_READ)
            {
                webserv::logger.log(webserv::Logger::INFO, "Read in socket");

                Job *job = reinterpret_cast<Job *>(eventList[i].udata);
                int ret = read(fd, buffer, BUFFER_SIZE);
                buffer[ret] = 0;
                job->request.append(buffer);

                if (ret < BUFFER_SIZE) {
                    // добавляем в ядро с евентом врайт
                    EV_SET(&chEvent, fd, EVFILT_WRITE, EV_ADD | EV_ONESHOT, 0, 0, job);
                    kevent(mKqueue, &chEvent, 1, NULL, 0, NULL);
                }
            }
            // пишем в сокет
            else if (eventList[i].filter == EVFILT_WRITE)
            {
                webserv::logger.log(webserv::Logger::INFO, "Write in socket");

                Job *job = reinterpret_cast<Job *>(eventList[i].udata);
                int res = write(fd, job->request.c_str(), job->request.size());
                job->request.clear();
            }
        }
    }
}

void webserv::EventPool::addListenSocket(webserv::Socket& socket) {
    struct kevent chEvent;

    EV_SET(&chEvent, socket.getListenSocket(), EVFILT_READ, EV_ADD | EV_CLEAR, 0, 0, NULL);
    int res = kevent(mKqueue, &chEvent, 1, NULL, 0, NULL);
    if (res == -1) {
        throw std::runtime_error("register socket descriptor for kqueue() fail");
    }
    mListenSockets.insert(std::pair<int, webserv::Socket&>(socket.getListenSocket(), socket));
    webserv::logger.log(webserv::Logger::INFO, "Add listen socket");
}
