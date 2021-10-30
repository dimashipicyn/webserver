//
// Created by Lajuana Bespin on 10/29/21.
//

#include <unistd.h>
#include "EventPool.h"
#include "Logger.h"

webserv::EventPool::EventPool()
: mKqueue(0)
{
    mKqueue = kqueue();
    if (mKqueue == -1) {
        throw std::runtime_error("EventPool: kqueue()");
    }
}

webserv::EventPool::~EventPool() {

}

void webserv::EventPool::runEventLoop() {
    struct kevent   chEvent;
    struct kevent   eventList[NUM_EVENTS];
    const int       BUFFER_SIZE = 1024;
    char            buffer[BUFFER_SIZE];

    webserv::logger.log(webserv::Logger::INFO, "Run event loop");
    while (true) {
        int nEvent = kevent(mKqueue, NULL, 0, eventList, NUM_EVENTS, NULL);
        if (nEvent == -1) {
            throw std::runtime_error("EventPool: kevent()");
        }
        for (int i = 0; i < nEvent; ++i) {
            if (eventList[i].flags & EV_ERROR) {
                webserv::logger.log(webserv::Logger::ERROR, "Event socket error");
            }

            int fd = (int)eventList[i].ident;
            std::cout << fd << std::endl;

            // если соединение закрыто
            if (eventList[i].flags & EV_EOF) {
                webserv::logger.log(webserv::Logger::INFO, "Event socket disconnect");
                close(fd); // Socket is automatically removed from the kq by the kernel.
            }

            // если сокет есть среди слушающих, принимаем новое соединение
            //else if (mFindSocket(fd).first)
            else if (fd == mListenSockets[0].getListenSocket())
            {
                Socket s = mListenSockets[0];
                std::cout << s.getListenSocket() << std::endl;
                //int newConnect = mFindSocket(fd).second.acceptConnection(); //FIXME throw ex
                int newConnect = s.acceptConnection(); //FIXME throw ex

                // добавляем в ядро новый дескриптор
                EV_SET(&chEvent, newConnect, EVFILT_READ, EV_ADD, 0, 0, 0);
                kevent(mKqueue, &chEvent, 1, NULL, 0, NULL);

                webserv::logger.log(webserv::Logger::INFO, "Add new connection");
            }
            else if (eventList[i].filter == EVFILT_READ) {
                webserv::logger.log(webserv::Logger::INFO, "Read in socket");
                int ret = read(fd, buffer, BUFFER_SIZE);
                buffer[ret] = 0;

                // добавляем в ядро с евентом врайт
                EV_SET(&chEvent, fd, EVFILT_WRITE, EV_ADD | EV_ONESHOT, 0, 0, NULL);
                kevent(mKqueue, &chEvent, 1, NULL, 0, NULL);
            }
            else if (eventList[i].filter == EVFILT_WRITE) {
                std::cout << "Write in socket " << fd << std::endl;
                char resp[] = "HTTP/1.1 200 OK\n"
                              "Connection: Keep-Alive\n"
                              "Date: Wed, 07 Sep 2016 00:38:13 GMT\n"
                              "Keep-Alive: timeout=5, max=1000\n"
                              "Content-Length: 12\n\n"
                              "Hello world\n";
                write(fd, resp, strlen(resp));
            }
        }
    }
}

std::pair<bool, const webserv::Socket> webserv::EventPool::mFindSocket(int fd) {
    std::vector<webserv::Socket>::iterator it = mListenSockets.begin();
    std::vector<webserv::Socket>::iterator ite = mListenSockets.end();

    while (it != ite) {
        if (it->getListenSocket() == fd) return std::make_pair(true, *it);
        it++;
    }
    return std::make_pair(false, *it);
}

void webserv::EventPool::addListenSocket(webserv::Socket& socket) {

    struct kevent chEvent;

    EV_SET(&chEvent, socket.getListenSocket(), EVFILT_READ, EV_ADD, 0, 0, NULL);
    int res = kevent(mKqueue, &chEvent, 1, NULL, 0, NULL);
    if (res == -1) {
        throw std::runtime_error("EventPool: addListenSocket: kevent()");
    }
    mListenSockets.push_back(socket);
    webserv::logger.log(webserv::Logger::INFO, "Add listen socket");
}
