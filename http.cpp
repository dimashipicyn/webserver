#include <iostream>

#include "http.h"
#include "EventPool.h"
#include "Request.h"

class Handler : public EventPool::IEventHandler {
    virtual void event(EventPool *evPool, std::uint16_t flags) {
        if ( flags & (EventPool::M_EOF | EventPool::M_ERROR) ) {
            std::cerr << "event error\n";
            evPool->removeEvent();
        }
        std::cout << "eventer\n";
    }
};


class ReadWriter
        : public EventPool::IEventReader
        , public EventPool::IEventWriter
{
public:
    ReadWriter(HTTP *http) : http(http) {}
    virtual void read(EventPool *evPool) {
        std::cout << "reader\n";
        int sock = evPool->eventGetSock();
        if (request.getState() == Request::READING) {
            std::cout << "reading" << std::endl;
            request.read(sock);
        }
        if (request.getState() == Request::ERROR) {
            evPool->removeEvent();
            std::cout << "error" << std::endl; // FIXME
        }
        if (request.getState() == Request::FINISH) {
            std::cout << "finish" << std::endl;
            evPool->eventSetFlags(EventPool::M_WRITE
                                  | EventPool::M_ADD
                                  | EventPool::M_ENABLE);
            //evPool->eventSetFlags(webserv::EventPool::M_TIMER
            //                      | webserv::EventPool::M_ADD
            //                      | webserv::EventPool::M_ENABLE
            //                      , 1000
            //                      );
        }
    }

    virtual void write(EventPool *evPool) {
        std::cout << request << std::endl;
        IHandle* h = http->getHandle(request.getPath());
        if (h) {
            h->handler(evPool->eventGetSock(), request);
        }
        request.reset();
        evPool->eventSetFlags(EventPool::M_WRITE | EventPool::M_DISABLE);
        evPool->eventSetFlags(EventPool::M_TIMER | EventPool::M_DISABLE);
    }

    Request request;
    HTTP    *http;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HTTP::HTTP(EventPool *evPool, const std::string& host)
    : evPool_(evPool), socket_(host, 1234), handleMap_()
{
    assert(evPool_); // user error

    socket_.makeNonBlock();
    socket_.listen();
    evPool_->addListener(socket_.getSock(), socket_.getAddr(), this);
}

HTTP::~HTTP()
{
    delete evPool_;
}

void HTTP::accept(EventPool *evPool, int sock, struct sockaddr *addr)
{
    int conn = ::accept(sock, addr, (socklen_t[]){sizeof(struct sockaddr)});
    evPool->addEvent(conn
                     , addr
                     , EventPool::M_READ
                     | EventPool::M_ADD);
    ReadWriter  *readWriter_ = new ReadWriter(this);
    Handler *h = new Handler;

    evPool->eventSetCb(nullptr, readWriter_, readWriter_, h);
    std::cout << "accept\n";
}

void HTTP::handle(const std::string& path, IHandle *h)
{
    try {
        handleMap_.insert(std::make_pair(path, h));
    }  catch (std::exception& e) {
        std::cout << e.what() << std::endl;
    }
}

IHandle *HTTP::getHandle(const std::string& path) {
    std::map<std::string, IHandle*>::iterator it = handleMap_.find(path);
    if (it != handleMap_.end()) {
        return it->second;
    }
    return nullptr; // FIXME, future return error page
}

void HTTP::start() {
    evPool_->start();
}

IHandle::~IHandle() {}
