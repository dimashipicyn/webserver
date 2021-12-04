#include <sys/event.h>
#include "TcpSocket.h"
#include "EventPool.h"

using namespace webserv;

void eventCallBack(EventPool *evp, EventPool::Event *event, std::uint16_t flags, std::uintptr_t ctx) {
    if ( flags & EV_ERROR || flags & EV_EOF ) {
        delete event; // is automatically removed from the kq by the kernel.
    }
}

void writeCallBack(EventPool *evp, EventPool::Event *event, std::uintptr_t ctx) {
    int sd = event->getSock();
    std::string *s = reinterpret_cast<std::string*>(ctx);
    ::write(sd, "Server: ", 8);
    ::write(sd, s->c_str(), s->size());
    delete s;
    evp->eventDisable(event, EventPool::WRITE);
    evp->eventDisable(event, EventPool::TIMER);
}

void readCallBack(EventPool *evp, EventPool::Event *event, std::uintptr_t ctx) {
    int sd = event->getSock();
    char buf[1024] = {0};
    read(sd, buf, 1024);
    std::cout << buf << std::endl;
    event->m_ctx = (std::uintptr_t)new std::string(buf);
    evp->eventEnable(event, EventPool::WRITE);
    evp->eventEnable(event, EventPool::TIMER, 5000);
}

void acceptCallBack(EventPool *evpool, int sd, struct sockaddr *addr) {

    int conn = ::accept(sd, addr, (socklen_t[]){sizeof(struct sockaddr)});
    EventPool::Event *event = new EventPool::Event(conn, addr);
    evpool->eventSetCb(event, readCallBack, writeCallBack, eventCallBack, 0);
    evpool->eventEnable(event, EventPool::READ);
}

int main()
{
    webserv::EventPool  eventPool;
    TcpSocket s("127.0.0.1", 1234);
    s.makeNonBlock();
    s.listen();
    eventPool.addListener(s.getSock(), s.getAddr(), acceptCallBack);
    eventPool.eventLoop();

    return 0;
}
