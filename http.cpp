#include <iostream>

#include "http.h"
#include "EventPool.h"
#include "Request.h"
#include "Response.h"

class Handler : public IEventHandler {
    virtual void event(EventPool *evPool, std::uint16_t flags) {
        if ( flags & EventPool::M_EOF  || flags & EventPool::M_ERROR ) {
            std::cerr << "event error\n";
            evPool->removeEvent();
        }
        std::cout << "eventer\n";
    }
};




struct Reader : public IEventReader
{
    Reader(HTTP& http) : request(), http(http) {

    }
    virtual void read(EventPool *evPool)
    {
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
            evPool->eventSetFlags(EventPool::M_TIMER
                                  | EventPool::M_ADD
                                  | EventPool::M_ENABLE
                                  , 1000
                                  );
        }
    }


    Request request;
    HTTP&    http;
};

struct Writer : public IEventWriter
{
    Writer(HTTP& http, Reader *reader) : response(), http(http), reader(reader) {

    }
    virtual void write(EventPool *evPool)
    {
        std::cout << reader->request << std::endl;
        IHandle* callback = http.getHandle(reader->request.getPath());
        if (callback) {
            callback->handler(reader->request, response);
        }

        // пишем в сокет
        int sock = evPool->eventGetSock();
        ::write(sock, response.getContent().c_str(), response.getContent().size());

        // сброс
        reader->request.reset();
        response.reset();

        // выключаем write
        evPool->eventSetFlags(EventPool::M_WRITE | EventPool::M_DISABLE);
        evPool->eventSetFlags(EventPool::M_TIMER | EventPool::M_DISABLE);
    }
    Response    response;
    HTTP&       http;
    Reader*     reader;
};

struct Accepter : public IEventAcceptor
{
    Accepter(HTTP& http) : http(http) {}

    virtual void accept(EventPool *evPool, int sock, struct sockaddr *addr)
    {
        int conn = ::accept(sock, addr, (socklen_t[]){sizeof(struct sockaddr)});
        std::cout << "new fd: " << conn << std::endl;
        evPool->addEvent(conn
                         , addr
                         , EventPool::M_READ
                         | EventPool::M_ADD);
        std::auto_ptr<IEventReader>  reader(new Reader(http));
        std::auto_ptr<IEventWriter>  writer(new Writer(http, static_cast<Reader*>(reader.get())));
        std::auto_ptr<IEventHandler> handler(new Handler);

        evPool->eventSetCb(std::auto_ptr<IEventAcceptor>(), reader, writer, handler);
        std::cout << "accept\n";
    }

    HTTP& http;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HTTP::HTTP(EventPool *evPool, const std::string& host, std::int16_t port)
    : evPool_(evPool), socket_(host, port), handleMap_()
{
    assert(evPool_); // user error

    socket_.makeNonBlock();
    socket_.listen();
    std::auto_ptr<IEventAcceptor> accepter(new Accepter(*this));
    evPool_->addListener(socket_.getSock(), socket_.getAddr(), accepter);
}

HTTP::~HTTP()
{

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
