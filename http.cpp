#include <iostream>
#include "Logger.h"
#include "http.h"
#include "EventPool.h"
#include "Request.h"
#include "Response.h"
#include "autoindex/Autoindex.hpp"

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
    Writer(HTTP& http, Reader *reader) : http(http), reader(reader) {

    }
    virtual void write(EventPool *evPool, Event *event)
    {
        LOG_DEBUG("Event writer call\n");

        http.handler(reader->request);

        // пишем в сокет
        int sock = event->sock;

		std::string output = http.getResponse().getContent();
		::write(sock, output.c_str(), output.size());

        // сброс
        reader->request.reset();

        // выключаем write
        evPool->addEvent(event, EventPool::M_WRITE | EventPool::M_DISABLE);
        //evPool->eventSetFlags(EventPool::M_TIMER | EventPool::M_DISABLE);
    }
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
void HTTP::handler(Request& request)
{

	std::string method = request.getMethod();
	if ( _method.count(method) ) (this->*_method.at(method))(request); // updating idea: here we can use try catch (if bad method) catch badrequest
	//also make pointers for all HTTP methods and in that methods implementation compare method with config allowed
	else if (_allMethods.count(method)) methodNotAllowed(request);
	else BadRequest();

/*	Response response(request);
	response_ = response;*/

    LOG_DEBUG("Http handler call\n");
    LOG_DEBUG("--------------PRINT REQUEST--------------\n");
    std::cout << request << std::endl;
/*
	// Сравниваем расширение запрошенного ресурса с cgi расширением для этого локейшена. Если бьется, запуск скрипта
	SettingsManager *settingsManager = SettingsManager::getInstance();
	Server *server = settingsManager->findServer(settingsManager->getHost().host, settingsManager->getHost()
	.port); // !!Здесь нужно передавать ip:port сервера, обрабатывающего запрос. Пока костыль
	std::string path = request.getPath();
	Route *route = server == nullptr ? nullptr : server->findRouteByPath(path);
	if (route != nullptr && utils::getExtension(path) == route->getCgi()) {
		response.setContent(Cgi(request).runCGI());
	}*/

}

void HTTP::start() {
    evPool_.start();
}

const Response& HTTP::getResponse() const{
	return response_;
}

//======http methods block moved from Response class============//
void HTTP::methodGET(const Request& request){

	LOG_DEBUG("Http handler call\n");
	LOG_DEBUG("--------------PRINT REQUEST--------------\n");
	std::cout << request << std::endl;
	SettingsManager *settingsManager = SettingsManager::getInstance();
	Server *server = settingsManager->findServer(settingsManager->getHost().host, settingsManager->getHost()
			.port); // !!Здесь нужно передавать ip:port сервера, обрабатывающего запрос. Пока костыль
	std::string path = request.getPath();
	Route *route = server == nullptr ? nullptr : server->findRouteByPath(path);
	if (route != nullptr && utils::getExtension(path) == route->getCgi()) {
		response_.setContent(Cgi(request).runCGI());
		return;
	}
	if (route != nullptr && route->isAutoindex() && utils::getExtension(path).empty()) {
		std::stringstream header;
		std::string html;
		try {
			html = Autoindex(*route).generatePage(path);
		} catch (std::runtime_error &e) {
			LOG_ERROR("%s\n", e.what());
			// Здесь респонс дефолтной ошибкой или той что указана в конфиге
			return;
		}
		response_.setStatusCode(200);
		response_.setHeaderField("Content-Length", html.size());
		response_.setContent(header.str() + html);
		std::ostringstream oss;
		oss << response_.getHeader();
		oss << html;
		response_.setContent(oss.str());
		return;
	}

	std::string finalContent;
	std::ifstream inputFile(path);
	if (!inputFile.good()){
		response_.setStatusCode(404);
		path = response_.getErrorPath(request);
		std::ifstream errorFile(path);
		if (!errorFile.good() ){
			response_.setStatusCode(500);
			return;
		} else {
			std::string errorContent((std::istreambuf_iterator<char>(errorFile)),
									 std::istreambuf_iterator<char>());
			finalContent = errorContent;
			response_.setHeaderField("Content-Type: ", "text/html");
		}
		errorFile.close();
	} else {
		response_.setStatusCode(200);
		std::string content((std::istreambuf_iterator<char>(inputFile)),
							std::istreambuf_iterator<char>());
		finalContent = content;
		inputFile.close();
		response_.setContentType(path);
	}
	response_.setHeaderField("Content-Length", finalContent.size() );
	std::ostringstream oss;
	oss << response_.getHeader();
	oss << finalContent;
	std::string output = oss.str(); // test unit would be deleted
	response_.setContent(oss.str());  // here is a output for GET method, this must to be
	// transfere to socket. Need I set response _output && pull it from response
	// again?
}


void HTTP::methodPOST(const Request& request){}
void HTTP::methodDELETE(const Request& request){}
void HTTP::methodNotAllowed(const Request& request){
	response_.setStatusCode(405);
	std::string strAllowMethods;
	for (std::map<std::string, void (HTTP::*)(const Request &)>::iterator it = _method.begin();
		it != _method.end(); ++it){
		if (it == _method.begin()) strAllowMethods += it->first;
		else strAllowMethods = strAllowMethods + ", " + it->first;
	}
	response_.setHeaderField("Allow", strAllowMethods);
}

<<<<<<< HEAD
void HTTP::BadRequest(){
	response_.setStatusCode(400);
=======
//    if (request.getMethod() == "GET" && request.getPath() == "/") {
//        std::stringstream ss;
//        std::string s("Hello Webserver!\n");
//        ss << "HTTP/1.1 200 OK\n"
//           << "Content-Length: " << s.size() << "\n"
//           << "Content-Type: text/html\n\n";
//        response.setContent(ss.str() + s);
//    }
>>>>>>> ca60e7898aa3228ed466167aed579f392938739b
}
//=============================================================//

//==============================Moved from Response class=====================
std::map<std::string, void (HTTP::*)(const Request &)>	HTTP::initMethods()
{
	std::map<std::string, void (HTTP::*)(const Request &)> map;
	map["GET"] = &HTTP::methodGET;
	map["POST"] = &HTTP::methodPOST;
	map["DELETE"] = &HTTP::methodDELETE;
	return map;
}

std::map<std::string, void (HTTP::*)(const Request &)> HTTP::_method
	= HTTP::initMethods();

std::set<std::string> HTTP::initAllMethods(){
	std::set<std::string> allMethods;
	allMethods.insert("GET");
	allMethods.insert("HEAD");
	allMethods.insert("POST");
	allMethods.insert("PUT");
	allMethods.insert("DELETE");
	allMethods.insert("CONNECT");
	allMethods.insert("OPTIONS");
	allMethods.insert("TRACE");
	allMethods.insert("PATCH");
	return allMethods;
}

std::set<std::string> HTTP::_allMethods = initAllMethods();
//=========================================================================