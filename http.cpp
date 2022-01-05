#include <iostream>
#include "Logger.h"
#include "http.h"
#include "EventPool.h"
#include "Request.h"
#include "Response.h"

class Handler : public IEventHandler {
    virtual void event(EventPool *evPool, Event *event, std::uint16_t flags) {
        LOG_DEBUG("Event handler call\n");
        if ( flags & EventPool::M_EOF  || flags & EventPool::M_ERROR ) {
            LOG_DEBUG("Event error or eof, fd: %d\n", event->sock);
            evPool->removeEvent(event);
        }
    }
};

struct Reader : public IEventReader
{
    Reader(HTTP& http) : request(), http(http) {

    }
    virtual void read(EventPool *evPool, Event *event)
    {
        LOG_DEBUG("Event reader call\n");

        int nBytes = ::read(event->sock, buffer, 1024);
        if (nBytes < 0) {
            LOG_ERROR("Error read socket: %d. %s\n", event->sock, ::strerror(errno));
        }
        buffer[nBytes] = '\0';
        request.parse(buffer);

        if (request.getState() == Request::PARSE_ERROR
         || request.getState() == Request::PARSE_DONE)
        {
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
    HTTP&   http;
    char    buffer[1025];
};

struct Writer : public IEventWriter
{
    Writer(HTTP& http, Reader *reader) : response(), http(http), reader(reader) {

    }
    virtual void write(EventPool *evPool, Event *event)
    {
        LOG_DEBUG("Event writer call\n");

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
        LOG_DEBUG("Event accepter call\n");
        int conn = ::accept(event->sock, event->addr, (socklen_t[]){sizeof(struct sockaddr)});

        LOG_DEBUG("New connect fd: %d\n", conn);

        Event *newEvent = new Event(conn, event->addr);

        std::auto_ptr<IEventReader>  reader(new Reader(http));
        std::auto_ptr<IEventWriter>  writer(new Writer(http, static_cast<Reader*>(reader.get())));
        std::auto_ptr<IEventHandler> handler(new Handler);

        newEvent->setCb(std::auto_ptr<IEventAcceptor>(), reader, writer, handler);
        evPool->addEvent(newEvent, EventPool::M_READ | EventPool::M_ADD | EventPool::M_ENABLE);
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
	// Прописываем адрес клиента в конфиге
	SettingsManager::getInstance()->setHost(host, port);

    TcpSocket socket(host, port);
    socket.makeNonBlock();
    socket.listen();
    std::auto_ptr<IEventAcceptor> accepter(new Accepter(*this));
    evPool_.addListener(socket.getSock(), socket.getAddr(), accepter);
}

HTTP::~HTTP()
{

}

// здесь происходит обработка запроса
void HTTP::handler(Request& request, Response& response)
{
    LOG_DEBUG("Http handler call\n");
    LOG_DEBUG("--------------PRINT REQUEST--------------\n");
    std::cout << request << std::endl;

	// Сравниваем расширение запрошенного ресурса с cgi расширением для этого локейшена. Если бьется, запуск скрипта
	SettingsManager *settingsManager = SettingsManager::getInstance();
	Server *server = settingsManager->findServer(settingsManager->getHost().host, settingsManager->getHost()
	.port);
	std::string path = request.getPath();
	Route *route = server == nullptr ? nullptr : server->findRouteByPath(path);
	if (route != nullptr && getExtension(path) == route->getCgi()) {
		response.setContent(Cgi(request).runCGI());
	}


    if (request.getMethod() == "GET" && request.getPath() == "/") {
        std::stringstream ss;
        std::string s("Hello Webserver!\n");
        ss << "HTTP/1.1 200 OK\n"
           << "Content-Length: " << s.size() << "\n"
           << "Content-Type: text/html\n\n";
        response.setContent(ss.str() + s);
    }
}


void HTTP::start() {
    evPool_.start();
}

