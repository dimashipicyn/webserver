#include <iostream>

#include "http.h"
#include "EventPool.h"
#include "Request.h"

class Handler : public webserv::EventPool::IEventHandler {
    virtual void event(webserv::EventPool *evPool, std::uint16_t flags) {
        if ( flags & (webserv::EventPool::M_EOF | webserv::EventPool::M_ERROR) ) {
            std::cerr << "event error\n";
        }
        std::cout << "eventer\n";
    }
};


class ReadWriter
        : public webserv::EventPool::IEventReader
        , public webserv::EventPool::IEventWriter
{
public:
    ReadWriter(HTTP *http) : http(http) {}
    virtual void read(webserv::EventPool *evPool) {
        std::cout << "reader\n";
        int sock = evPool->eventGetSock();
        if (request.getState() == Request::READING) {
            std::cout << "reading" << std::endl;
            request.read(sock);
        }
        if (request.getState() == Request::ERROR) {
            std::cout << "error" << std::endl; // FIXME
        }
        if (request.getState() == Request::FINISH) {
            std::cout << "finish" << std::endl;
            evPool->eventSetFlags(webserv::EventPool::M_WRITE
                                  | webserv::EventPool::M_ADD
                                  | webserv::EventPool::M_ENABLE);
            //evPool->eventSetFlags(webserv::EventPool::M_TIMER
            //                      | webserv::EventPool::M_ADD
            //                      | webserv::EventPool::M_ENABLE
            //                      , 1000
            //                      );
        }
    }

    virtual void write(webserv::EventPool *evPool) {
        std::cout << request << std::endl;
        IHandle* h = http->getHandle(request.getPath());
        if (h) {
            h->handler(evPool->eventGetSock(), request);
        }
        request.reset();
        evPool->eventSetFlags(webserv::EventPool::M_WRITE | webserv::EventPool::M_DISABLE);
        evPool->eventSetFlags(webserv::EventPool::M_TIMER | webserv::EventPool::M_DISABLE);
    }

    Request request;
    HTTP    *http;
};

class Accepter : public webserv::EventPool::IEventAcceptor {
public:
    Accepter(HTTP *http) : http_(http) {
    }
    ~Accepter() {
    }

    virtual void accept(webserv::EventPool *evPool, int sock, struct sockaddr *addr) {
        int conn = ::accept(sock, addr, (socklen_t[]){sizeof(struct sockaddr)});
        evPool->addEvent(conn
                         , addr
                         , webserv::EventPool::M_READ
                         | webserv::EventPool::M_ADD);

        ReadWriter  *readWriter_ = new ReadWriter(http_);

        evPool->eventSetCb(nullptr, readWriter_, readWriter_, nullptr);
        std::cout << "accept\n";
    }

private:
    HTTP  *http_;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HTTP::HTTP(const std::string& host)
    : evpool_(nullptr), socket_(host, 1234), handleMap_()
{
    evpool_ = new webserv::EventPool;
    accepter_ = new Accepter(this);

    socket_.makeNonBlock();
    socket_.listen();
    evpool_->addListener(socket_.getSock(), socket_.getAddr(), accepter_);
}

HTTP::~HTTP()
{
    delete evpool_;
    delete accepter_;
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
    evpool_->start();
}
