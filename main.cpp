#include <sys/event.h>
#include "Listener.h"
#include "EventPool.h"

using namespace webserv;

void eventCallBack(EventPool *evp, EventPool::Event *event, std::uint16_t flags, std::uintptr_t ctx) {
    if ( flags & EV_ERROR || flags & EV_EOF ) {
        delete event; // is automatically removed from the kq by the kernel.
    }
}

void writeCallBack(EventPool *evp, EventPool::Event *event, std::uintptr_t ctx) {
    int sd = event->getSd();
    char *msg = "hello world";
    ::write(sd, msg, strlen(msg));
    evp->eventDisable(event, EventPool::WRITE);
}

void readCallBack(EventPool *evp, EventPool::Event *event, std::uintptr_t ctx) {
    int sd = event->getSd();
    char buf[1024] = {0};
    read(sd, buf, 1024);
    std::cout << buf << std::endl;
    evp->eventEnable(event, EventPool::WRITE);
}

void acceptCallBack(EventPool *evt, int sd, struct sockaddr *addr) {

    int conn = ::accept(sd, addr, (socklen_t[]){sizeof(struct sockaddr)});
    webserv::EventPool::Event *event = new webserv::EventPool::Event(conn);
    event->setCb(readCallBack, writeCallBack, eventCallBack, 0);
    evt->addEvent(event);
    evt->eventEnable(event, EventPool::READ);
}

int main()
{
    webserv::EventPool  eventPool;

    eventPool.addListenSocket(0, acceptCallBack);
    eventPool.eventLoop();

    return 0;
}
