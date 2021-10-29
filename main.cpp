#include <iostream>
#include <unistd.h>

#include <vector>
#include <sys/event.h>

#include "HttpServer.h"
#include "EventPool.h"

#define BUFSIZE 1024

int main()
{
    webserv::HttpServer server("127.0.0.1", 1234);
    webserv::EventPool  eventPool;



    struct kevent chevent;

    char buf[BUFSIZE];

    EV_SET(&chevent, server.getListenSocket(), EVFILT_READ, EV_ADD | EV_CLEAR, 0, 0, 0);
    eventPool.addEvent(chevent);
    while (true) {
        std::vector<struct kevent> ev = eventPool.getEvents();
        std::cout << ev.size() << std::endl;
        for (int i = 0; i < ev.size(); ++i) {
            if (ev[i].flags & EV_ERROR) {
                std::cout << "Error: " << ev[i].ident << std::endl;
            }

            int fd = (int)ev[i].ident;

            if (ev[i].flags & EV_EOF) {
                std::cout << "Disconnect\n" << std::endl;
                close(fd);
                // Socket is automatically removed from the kq by the kernel.
            }
            else if (fd == server.getListenSocket()) {
                int f = server.acceptConnection();
                std::cout << "Add new connection in " << fd << " new f " << f << std::endl;
                EV_SET(&chevent, f, EVFILT_READ, EV_ADD, 0, 0, 0);
                eventPool.addEvent(chevent);
            }
            else if (ev[i].filter == EVFILT_READ) {
                std::cout << "Read in socket " << fd << std::endl;
                int ret = read(fd, buf, BUFSIZE);
                buf[ret] = 0;
                std::cout << "bytes: " << ret << "\n" << buf << std::endl;
                EV_SET(&chevent, fd, EVFILT_WRITE, EV_ADD | EV_ONESHOT, 0, 0, NULL);
                eventPool.addEvent(chevent);
            }
            else if (ev[i].filter == EVFILT_WRITE) {
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

    return 0;
}
