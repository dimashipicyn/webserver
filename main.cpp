#include <sys/event.h>
#include "TcpSocket.h"
#include "EventPool.h"

class Handler : public webserv::IEventHandler {
    virtual void event(webserv::EventPool *evPool, std::uint16_t flags) {
        if ( flags & EV_ERROR || flags & EV_EOF ) {
            std::cerr << "event error\n";
        }
        std::cout << "eventer\n";
    }
};

class Reader;

class Writer : public webserv::IEventWriter {
public:
    virtual void write(webserv::EventPool *evPool) {
    int sock = evPool->eventGetSock();
    ::write(sock, "Server: ", 8);
    ::write(sock, buffer.c_str(), buffer.size());
    buffer.clear();
    evPool->eventDisable(webserv::EventPool::WRITE);
    evPool->eventDisable(webserv::EventPool::TIMER);
    std::cout << "writer\n";
    }

    Reader *reader_;
    std::string buffer;
};

class Reader : public webserv::IEventReader {
public:
    virtual void read(webserv::EventPool *evPool) {
        char buf[1024] = {0};
        int sock = evPool->eventGetSock();
        ::read(sock, buf, 1024);
        std::cout << buf << std::endl;
        writer_->buffer.append(buf);
        evPool->eventEnable(webserv::EventPool::WRITE);
        evPool->eventEnable(webserv::EventPool::TIMER, 5000);
        std::cout << "reader\n";
    }

    Writer *writer_;
    std::string buffer;
};


class Accepter : public webserv::IEventAcceptor {
public:
    virtual void accept(webserv::EventPool *evPool, int sock, struct sockaddr *addr) {
        int conn = ::accept(sock, addr, (socklen_t[]){sizeof(struct sockaddr)});
        evPool->addEvent(conn, addr);
        Reader *reader = new Reader;
        Writer *writer = new Writer;
        Handler *handler = new Handler;

        reader->writer_ = writer;
        writer->reader_ = reader;
        evPool->eventSetCb(nullptr, reader, writer, handler);
        evPool->eventEnable(webserv::EventPool::READ);
        std::cout << "accept\n";
    }
};

int main()
{
    webserv::EventPool  eventPool;
    webserv::TcpSocket s("127.0.0.1", 1234);
    s.makeNonBlock();
    s.listen();
    eventPool.addListener(s.getSock(), s.getAddr(), new Accepter);
    eventPool.eventLoop();

    return 0;
}
