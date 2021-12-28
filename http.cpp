#include <iostream>

#include "http.h"
#include "EventPool.h"
#include "Request.h"
#include "Response.h"

class Handler : public IEventHandler {
    virtual void event(EventPool *evPool, Event *event, std::uint16_t flags) {
        if ( flags & EventPool::M_EOF  || flags & EventPool::M_ERROR ) {
            std::cerr << "event error\n";
            evPool->removeEvent(event);
        }
        std::cout << "eventer\n";
    }
};

struct Reader : public IEventReader
{
    Reader(HTTP& http) : request(), http(http) {

    }
    virtual void read(EventPool *evPool, Event *event)
    {
        std::cout << "reader\n";
        int sock = event->sock;
        if (request.getState() == Request::READING) {
            std::cout << "reading" << std::endl;
            request.read(sock);
        }
        if (request.getState() == Request::ERROR) {
            evPool->removeEvent(event);
            std::cout << "error" << std::endl; // FIXME
        }
        if (request.getState() == Request::FINISH) {
            std::cout << "finish" << std::endl;
            evPool->addEvent(event, EventPool::M_WRITE
                                  | EventPool::M_ADD
                                  | EventPool::M_ENABLE);
            //evPool->eventSetFlags(EventPool::M_TIMER
            //                      | EventPool::M_ADD
             //                     | EventPool::M_ENABLE
            //                      , 1000
            //                      );
        }
    }


    Request request;
    HTTP&    http;
};

struct Writer : public IEventWriter
{
    Writer(HTTP& http, Reader *reader) : response(), http(http), reader(reader) {

    }
    virtual void write(EventPool *evPool, Event *event)
    {
        std::cout << reader->request << std::endl;
        http.handler(reader->request, response);

        // пишем в сокет
        int sock = event->sock;
        ::write(sock, response.getContent().c_str(), response.getContent().size());

        // сброс
        reader->request.reset();
        //response.reset();

        // выключаем write
        evPool->addEvent(event, EventPool::M_WRITE | EventPool::M_DISABLE);
        //evPool->eventSetFlags(EventPool::M_TIMER | EventPool::M_DISABLE);
    }
    Response    response;
    HTTP&       http;
    Reader*     reader;
};

struct Accepter : public IEventAcceptor
{
    Accepter(HTTP& http) : http(http) {}

    virtual void accept(EventPool *evPool, Event *event)
    {
        int conn = ::accept(event->sock, event->addr, (socklen_t[]){sizeof(struct sockaddr)});
        std::cout << "new fd: " << conn << std::endl;

        Event *newEvent = new Event(conn, event->addr);

        std::auto_ptr<IEventReader>  reader(new Reader(http));
        std::auto_ptr<IEventWriter>  writer(new Writer(http, static_cast<Reader*>(reader.get())));
        std::auto_ptr<IEventHandler> handler(new Handler);

        newEvent->setCb(std::auto_ptr<IEventAcceptor>(), reader, writer, handler);
        evPool->addEvent(newEvent, EventPool::M_READ | EventPool::M_ADD);
        std::cout << "accept\n";
    }

    HTTP& http;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HTTP::HTTP(const std::string& host, std::int16_t port)
{
    TcpSocket socket(host, port);
    socket.makeNonBlock();
    socket.listen();
    std::auto_ptr<IEventAcceptor> accepter(new Accepter(*this));
    evPool_.addListener(socket.getSock(), socket.getAddr(), accepter);
	std::string methodsIn[] = {"GET", "HEAD","POST", "PUT", "DELETE", "CONNECT",
							   "OPTIONS", "TRACE", "PATCH", };
	for (int i = 0; i < 9; ++i) _methods.insert(methodsIn[i]);
}

HTTP::~HTTP()
{

}



// здесь происходит обработка запроса
void HTTP::handler(Request& request, Response& response)
{
	std::string method = "GET"; // method = request.method;

	if (method == "GET") return methodGET(request, response);
	else if (method == "POST") return methodPOST(request, response);
	else if (method == "DELETE") return methodDELETE(request, response);
	else if (_methods.count(method) != 0) return methodNotAllowed();
}



void HTTP::methodGET(Request& request, Response& response){
	response.errorPage();
}

void HTTP::methodPOST(Request& request, Response& response) {

}

void HTTP::methodDELETE(Request& request, Response& response) {

}

void HTTP::methodNotAllowed(){

}

void HTTP::start() {
    evPool_.start();
}

