#include <iostream>
#include <array>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>


#include "Logger.h"
#include "http.h"
#include "Request.h"
#include "Response.h"
#include "Autoindex.hpp"
#include "Route.hpp"
#include "Server.hpp"
#include "utils.h"
#include "TcpSocket.h"
#include "SettingsManager.hpp"
#include "Cgi.hpp"
#include "httpExceptions.h"

const int64_t sessionTimeout = 250000; // 250 sec

typedef void (HTTP::*handlerFunc)(int, Session*);

struct Session
{
	void bind(handlerFunc f) {
		func = f;
	};

	std::string readBuf;
	std::string writeBuf;

	handlerFunc func;
	int32_t     fileDesc;
	int64_t     fileSize;
	int64_t     recvBytes;
	Request     request;
};

HTTP::HTTP()
		: sessionMap_()
{

}

HTTP::~HTTP()
{

}

void HTTP::start() {
	EventPool::start();
}

void HTTP::listen(const std::string& host)
{
	try {
		TcpSocket conn(host);
		conn.listen();
		conn.makeNonBlock();

		Session listenSession;
		newListenerEvent(conn);
		newSessionByID(conn.getSock(), listenSession);
	}  catch (std::exception& e) {
		LOG_ERROR("HTTP: %s\n", e.what());
	}
}

Session* HTTP::getSessionByID(int id)
{
	if (sessionMap_.find(id) != sessionMap_.end()) {
		return &sessionMap_[id];
	}
	return nullptr;
}

void HTTP::closeSessionByID(int id) {
	disableTimerEvent(id, 0);
	::close(id);
	sessionMap_.erase(id);
}

void HTTP::newSessionByID(int id, Session& session)
{
	sessionMap_[id] = session;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////// ASYNC LOGIC /////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void HTTP::asyncAccept(TcpSocket& socket)
{
	LOG_DEBUG("Event accepter call\n");

	try
	{
		Session *session = getSessionByID(socket.getSock());
		if (session == nullptr) {
			LOG_ERROR("Dont find session to socket: %d. Close.\n", socket.getSock());
			close(socket.getSock());
			return;
		}

		TcpSocket conn = socket.accept();

		LOG_DEBUG("New connect fd: %d\n", conn.getSock());

		Session s;
		s.bind(&HTTP::defaultReadFunc);
		s.request.setHost(conn.getHost());

		newSessionByID(conn.getSock(), s);
		enableReadEvent(conn.getSock());
		enableTimerEvent(conn.getSock(), sessionTimeout); // TODO add config
	}
	catch (std::exception& e)
	{
		LOG_ERROR("HTTP::asyncAccept error: %s\n", e.what());
	}
};

void HTTP::asyncWrite(int socket)
{
	LOG_DEBUG("Event writer call\n");

	Session *session = getSessionByID(socket);
	if (session == nullptr) {
		LOG_ERROR("Dont find session to socket: %d. Close connection.\n", socket);
		closeSessionByID(socket);
		return;
	}
	(this->*session->func)(socket, session);
}

void HTTP::asyncRead(int socket)
{
	LOG_DEBUG("Event reader call\n");

	Session *session = getSessionByID(socket);
	if (session == nullptr) {
		LOG_ERROR("Dont find session to socket: %d. Close connection.\n", socket);
		closeSessionByID(socket);
		return;
	}
	(this->*session->func)(socket, session);
}

void HTTP::asyncEvent(int socket, uint16_t flags)
{
	LOG_DEBUG("Event handler call\n");
	if (flags & EventPool::M_EOF || flags & EventPool::M_ERROR) {
		LOG_ERROR("Event error or eof, socket: %d\n", socket);
		closeSessionByID(socket);
	}
	if (flags & EventPool::M_TIMER) {
		LOG_ERROR("Event timeout, socket: %d\n", socket);
		closeSessionByID(socket);
	}
}

void HTTP::defaultReadFunc(int socket, Session *session)
{
	LOG_DEBUG("defaultReadFunc call\n");
	const int64_t bufSize = (1 << 16);
	char buf[bufSize];

	int64_t readBytes = ::read(socket, buf, bufSize - 1);
	if (readBytes < 0) {
		closeSessionByID(socket);
		LOG_ERROR("Dont read to socket: %d. Close connection.\n", socket);
		return;
	}
	buf[readBytes] = '\0';
	session->readBuf.append(buf);
	if (session->readBuf.find("\n\n") != std::string::npos
		|| session->readBuf.find("\r\n\r\n") != std::string::npos)
	{
		session->bind(&HTTP::defaultWriteFunc);
		// включаем write
		enableWriteEvent(socket);
		disableReadEvent(socket);
	}
}

void HTTP::defaultWriteFunc(int socket, Session *session)
{
	LOG_DEBUG("defaultWriteFunc call\n");
	session->bind(&HTTP::doneFunc);

	std::string& rbuf = session->readBuf;
	Request& request = session->request;

	request.reset();
	request.parse(rbuf);
	request.setID(socket);

	rbuf.clear();

	Response response;
	handler(request, response);

	std::string& wbuf = session->writeBuf;
	wbuf.append(response.getContent());

	int64_t writeBytes = ::write(socket, wbuf.c_str(), wbuf.size());

	if (writeBytes < 0) {
		closeSessionByID(socket);
		LOG_ERROR("Dont write to socket: %d. Close connection.\n", socket);
		return;
	}

	wbuf.erase(0, writeBytes);
}

void HTTP::doneFunc(int socket, Session *session)
{
	LOG_DEBUG("doneFunc call\n");
	std::string& wbuf = session->writeBuf;

	if (wbuf.empty()) {
		disableWriteEvent(socket);
		enableReadEvent(socket);
		session->bind(&HTTP::defaultReadFunc);
	}
	else {
		int64_t writeBytes = ::write(socket, wbuf.c_str(), wbuf.size());

		if (writeBytes < 0) {
			closeSessionByID(socket);
			LOG_ERROR("Dont write to socket: %d. Close connection.\n", socket);
			return;
		}

		wbuf.erase(0, writeBytes);
	}
}

void HTTP::sendFileFunc(int socket, Session* session)
{
	LOG_DEBUG("sendFileFunc call\n");
	const int32_t bufSize = (1 << 16);
	char buf[bufSize] = {};

	int32_t fd = session->fileDesc;
	int32_t readBytes = ::read(fd, buf, bufSize - 1);

	if (readBytes == 0) {
		close(fd);
		session->bind(&HTTP::doneFunc);
	}

	if (readBytes < 0) {
		closeSessionByID(socket);
		LOG_ERROR("Dont read to file: %d. Close connection.\n", socket);
		return;
	}

	buf[readBytes] = '\0';

	std::string& wbuf = session->writeBuf;
	wbuf.append(buf);

	int32_t writeBytes = ::write(socket, wbuf.c_str(), wbuf.size());

	if (writeBytes < 0) {
		closeSessionByID(socket);
		LOG_ERROR("Dont write to socket: %d. Close connection.\n", socket);
		return;
	}

	wbuf.erase(0, writeBytes);
}

void HTTP::recvFileFunc(int socket, Session* session)
{
	LOG_DEBUG("recvFileFunc call\n");
	const int32_t bufSize = (1 << 16);
	char buf[bufSize] = {};


	int32_t readBytes = ::read(socket, buf, bufSize - 1);

	if (readBytes < 0) {
		closeSessionByID(socket);
		LOG_ERROR("Dont read to file: %d. Close connection.\n", socket);
		return;
	}

	buf[readBytes] = '\0';


	std::string& wbuf = session->writeBuf;
	wbuf.append(buf);

	int32_t fd = session->fileDesc;

	int64_t bytes = 0;
	if (session->fileSize != -1) {
		bytes = std::min<int64_t>(wbuf.size(), session->fileSize - session->recvBytes);
	}
	else {
		bytes = wbuf.size();
	}


	int32_t writeBytes = ::write(fd, wbuf.c_str(), bytes);

	if (writeBytes < 0) {
		closeSessionByID(socket);
		LOG_ERROR("Dont write to socket: %d. Close connection.\n", socket);
		return;
	}

	session->recvBytes += writeBytes;
	if (session->recvBytes == session->fileSize) {
		session->bind(&HTTP::cgiCaller);
		disableReadEvent(socket);
		enableWriteEvent(socket);
	}

	if (session->fileSize == -1 && wbuf.size()) {
		session->bind(&HTTP::cgiCaller);
		disableReadEvent(socket);
		enableWriteEvent(socket);
	}

	wbuf.erase(0, writeBytes);
}


void HTTP::cgiCaller(int socket, Session* session)
{
	LOG_DEBUG("CgiCaller call\n");
	Request& request = session->request;

	Response response;
	cgi(request, response, nullptr);

	session->writeBuf.append(response.getContent());
	session->bind(&HTTP::doneFunc);
}

///////////////////////////////////////////////////////////////////////////
///////////////////////// HTTP LOGIC //////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

void HTTP::cgi(const Request &request, Response& response, Route* route) {
	const std::string& path = request.getPath();

	if (route != nullptr && utils::getExtension(path) == route->getCgi()) {
		response.setContent(Cgi(request, *route).runCGI());
	}
}

void HTTP::autoindex(const Request &request, Response &response, Route *route) {
	const std::string& path = request.getPath();

	if (route != nullptr && route->isAutoindex() && utils::getExtension(path).empty()) {
		std::stringstream header;
		std::string html;
		try
		{
			html = route->getDefaultPage(path);
		} catch (Route::DefaultFileNotFoundException &e) {
			LOG_DEBUG("Default file at %s not found. Proceed autoindexing.\n", path.c_str());
			html = Autoindex(*route).generatePage(path);
		}
		header << "HTTP/1.1 200 OK\n"
			   << "Content-Length: " << html.size() << "\n"
			   << "Content-Type: text/html\n\n";
		response.setContent(header.str() + html);
	}
}

// здесь происходит обработка запроса
void HTTP::handler(Request& request, Response& response) {
	try {
		LOG_DEBUG("Http handler call\n");
		LOG_DEBUG("--------------PRINT REQUEST--------------\n");
		std::cout << request << std::endl;


//		if (request.getMethod() == "GET") {
//			sendFile(request, response, "./index.html");
//			return;
//		}
//		if (request.getMethod() == "POST") {
//			recvFile(request, response, "./my_file");
//			return;
//		}


		// Сравниваем расширение запрошенного ресурса с cgi расширением для этого локейшена. Если бьется, запуск скрипта
		SettingsManager *settingsManager = SettingsManager::getInstance();

		Server *server = settingsManager->findServer(request.getHost());
		Route *route = (server == nullptr ? nullptr : server->findRouteByPath(request.getPath()));
		if (route == nullptr) {
			throw httpEx<NotFound>("Not Found");
		}
		std::string method = request.getMethod();
		if (_method.count(method))
			(this->*_method.at(method))(
					request, response, route); // updating idea: here we can use try catch (if bad method) catch badrequest
			//also make pointers for all HTTP methods and in that methods implementation compare method with config allowed
		else if (_allMethods.count(method)) {
			methodNotAllowed(request, response);
		}
		else {
			throw httpEx<BadRequest>("Bad Request");
		}
	}
	catch (httpEx<BadRequest> &e) {
		LOG_INFO("BadRequest: %s\n", e.what());
	}
	catch (httpEx<Unauthorized> &e) {
		LOG_INFO("Unauthorized: %s\n", e.what());
	}
	catch (httpEx<PaymentRequired> &e) {
		LOG_INFO("PaymentRequired: %s\n", e.what());
	}
	catch (httpEx<Forbidden> &e) {
		LOG_INFO("Forbidden: %s\n", e.what());
	}
	catch (httpEx<NotFound> &e) {
		LOG_INFO("NotFound: %s\n", e.what());
		response.buildErrorPage(e.error_code, request);
	}
	catch (httpEx<MethodNotAllowed> &e) {
		LOG_INFO("MethodNotAllowed: %s\n", e.what());
	}
	catch (httpEx<NotAcceptable> &e) {
		LOG_INFO("NotAcceptable: %s\n", e.what());
	}
	catch (httpEx<ProxyAuthenticationRequired> &e) {
		LOG_INFO("ProxyAuthenticationRequired: %s\n", e.what());
	}
	catch (httpEx<RequestTimeout> &e) {
		LOG_INFO("RequestTimeout: %s\n", e.what());
	}
	catch (httpEx<Conflict> &e) {
		LOG_INFO("Conflict: %s\n", e.what());
	}
	catch (httpEx<Gone> &e) {
		LOG_INFO("Gone: %s\n", e.what());
	}
	catch (httpEx<LengthRequired> &e) {
		LOG_INFO("LengthRequired: %s\n", e.what());
	}
	catch (httpEx<PreconditionFailed> &e) {
		LOG_INFO("PreconditionFailed: %s\n", e.what());
	}
	catch (httpEx<PayloadToLarge> &e) {
		LOG_INFO("PayloadToLarge: %s\n", e.what());
	}
	catch (httpEx<UriTooLong> &e) {
		LOG_INFO("UriTooLong: %s\n", e.what());
	}
	catch (httpEx<UnsupportedMediaType> &e) {
		LOG_INFO("UnsupportedMediaType: %s\n", e.what());
	}
	catch (httpEx<RangeNotSatisfiable> &e) {
		LOG_INFO("RangeNotSatisfiable: %s\n", e.what());
	}
	catch (httpEx<ExpectationFailed> &e) {
		LOG_INFO("ExpectationFailed: %s\n", e.what());
	}
	catch (httpEx<ImATeapot> &e) {
		LOG_INFO("ImATeapot: %s\n", e.what());
	}
	catch (httpEx<UnprocessableEntity> &e) {
		LOG_INFO("UnprocessableEntity: %s\n", e.what());
	}
	catch (httpEx<TooEarly> &e) {
		LOG_INFO("TooEarly: %s\n", e.what());
	}
	catch (httpEx<UpgradeRequired> &e) {
		LOG_INFO("UpgradeRequired: %s\n", e.what());
	}
	catch (httpEx<PreconditionRequired> &e) {
		LOG_INFO("PreconditionRequired: %s\n", e.what());
	}
	catch (httpEx<TooManyRequests> &e) {
		LOG_INFO("TooManyRequests: %s\n", e.what());
	}
	catch (httpEx<RequestHeaderFieldsTooLarge> &e) {
		LOG_INFO("RequestHeaderFieldsTooLarge: %s\n", e.what());
	}
	catch (httpEx<UnavailableForLegalReasons> &e) {
		LOG_INFO("UnavailableForLegalReasons: %s\n", e.what());
	}
	catch (httpEx<InternalServerError> &e) {
		LOG_INFO("InternalServerError: %s\n", e.what());
	}
	catch (httpEx<NotImplemented> &e) {
		LOG_INFO("NotImplemented: %s\n", e.what());
	}
	catch (httpEx<BadGateway> &e) {
		LOG_INFO("BadGateway: %s\n", e.what());
	}
	catch (httpEx<ServiceUnavailable> &e) {
		LOG_INFO("ServiceUnavailable: %s\n", e.what());
	}
	catch (httpEx<GatewayTimeout> &e) {
		LOG_INFO("GatewayTimeout: %s\n", e.what());
	}
	catch (httpEx<HTTPVersionNotSupported> &e) {
		LOG_INFO("HTTPVersionNotSupported: %s\n", e.what());
	}
	catch (httpEx<VariantAlsoNegotiates> &e) {
		LOG_INFO("VariantAlsoNegotiates: %s\n", e.what());
	}
	catch (httpEx<InsufficientStorage> &e) {
		LOG_INFO("InsufficientStorage: %s\n", e.what());
	}
	catch (httpEx<LoopDetected> &e) {
		LOG_INFO("LoopDetected: %s\n", e.what());
	}
	catch (httpEx<NotExtended> &e) {
		LOG_INFO("NotExtended: %s\n", e.what());
	}
	catch (httpEx<NetworkAuthenticationRequired> &e) {
		LOG_INFO("NetworkAuthenticationRequired: %s\n", e.what());
	}
}

//=======================Moved from web.1.0 ===================================

void HTTP::methodGET(const Request& request, Response& response, Route* route)
{
	cgi(request, response, route);
	autoindex(request, response, route);
	if (!response.getContent().empty()) return; // Костыльно, но времени не хватит для более глубокой интеграции

	//read source file
	std::string path = route->getFullPath(request.getPath());
	std::string body = response.readFile(path);

	//	std::string errorPage = SettingsManager::getInstance()->findServer(request.getHost())->getErrorPage();// if not empty use config errorfile

	response.setStatusCode(200);
	response.setHeaderField("Host", request.getHost());
	response.setContentType(path);
	response.setHeaderField("Content-Length", body.size() );
	response.setContent(response.getHeader() + body);
}

void HTTP::methodPOST(const Request& request, Response& response, Route* route){/*
	//watch for methods allowed
		std::cout << request << std::endl;

		/*
		 * put autoindex && cgi her
		 */

	//read source file
/*		std::string path = route->getFullPath(request.getPath());
		std::string body = response.writeFile(path);
		response.setStatusCode(200);
		response.setHeaderField("Host", request.getHost());
		response.setContentType(path);
		response.setHeaderField("Content-Length", body.size() );
		response.setContent(response.getHeader() + body);*/
}

void HTTP::methodDELETE(const Request& request, Response& response, Route* route){}

void HTTP::methodPUT(const Request & request, Response & response, Route *) {}

void HTTP::methodHEAD(const Request &, Response &, Route *) {}
void HTTP::methodNotAllowed(const Request& request, Response& response){
	response.setStatusCode(405);
	std::string strAllowMethods;
	for (HTTP::MethodHttp::iterator it = _method.begin();
		 it != _method.end(); ++it){
		if (it == _method.begin()) strAllowMethods += it->first;
		else strAllowMethods = strAllowMethods + ", " + it->first;
	}
	response.setHeaderField("Allow", strAllowMethods);
}

/*
	void HTTP::BadRequest(Response& response){
		response.setStatusCode(400);
		std::stringstream ss;
		std::string s("Bad Request\n");
		response.setHeaderField("Content-Length", s.size());
		response.setContentType("text/html");
		response.setContent(response.getHeader() + s);
	}*/
//=============================================================//

//==============================Moved from Response class=====================


HTTP::MethodHttp 	HTTP::initMethods()
{
	std::map<std::string, void (HTTP::*)(const Request &, Response&, Route*)> map;
	map["GET"] = &HTTP::methodGET;
	map["POST"] = &HTTP::methodPOST;
	map["DELETE"] = &HTTP::methodDELETE;
	map["PUT"] = &HTTP::methodPUT;
	map["HEAD"] = &HTTP::methodHEAD;
	return map;
}



HTTP::MethodHttp HTTP::_method
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


void HTTP::startServer()
{
	std::vector<Server> servers = SettingsManager::getInstance()->getServers();
	HTTP serve;
	for (std::vector<Server>::const_iterator i = servers.begin(); i != servers.end(); i++) {
		char port[6];
		char ipstr[INET_ADDRSTRLEN];
		struct sockaddr_in sa;
		bzero(port, 6);
		sprintf(port, "%u", (*i).getPort());
		if (inet_pton(AF_INET, (*i).getHost().c_str(), &(sa.sin_addr)) < 1) {
			int status;
			struct addrinfo hints, *res, *p;
			memset(&hints, 0, sizeof hints);
			hints.ai_family = AF_INET;
			hints.ai_socktype = SOCK_STREAM;
			if ((status = getaddrinfo((*i).getHost().c_str(), port, &hints, &res)) != 0) {
				LOG_ERROR("Cannot get address from %s. Skipping...\n", (*i).getHost().c_str());
				continue;
			}
			for(p = res;p != NULL; p = p->ai_next) {
				void *addr;
				struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
				addr = &(ipv4->sin_addr);
				inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);

			}
			freeaddrinfo(p);
			freeaddrinfo(res);
		} else {
			strcpy(ipstr, (*i).getHost().c_str());
		}
		serve.listen(ipstr + std::string(":") + port);
	}
	serve.start();
}


void HTTP::sendFile(Request& request, Response& response, const std::string& path)
{
	int fd = ::open(path.c_str(), O_RDONLY);

	if (fd == -1) {
		throw httpEx<InternalServerError>("Cannot open file: " + path);
	}

	struct stat info;

	if (::fstat(fd, &info) == -1) {
		throw httpEx<InternalServerError>("Cannot stat file info: " + path);
	}

	int16_t sizeFile = info.st_size;

	response.setStatusCode(200);
	response.setHeaderField("Content-Length", sizeFile);
	response.setContent(response.getHeader());

	Session* session = getSessionByID(request.getID());

	session->fileDesc = fd;
	session->bind(&HTTP::sendFileFunc);
}

void HTTP::recvFile(Request& request, Response& response, const std::string& path)
{
	int fd = ::open(path.c_str(), O_WRONLY|O_CREAT, 0664);

	if (fd == -1) {
		throw httpEx<InternalServerError>("Cannot open file: " + path);
	}

	Session* session = getSessionByID(request.getID());

	session->fileDesc = fd;
	session->recvBytes = 0;
	if (request.hasHeader("Content-Length")) {
		session->fileSize = utils::to_number<int64_t>(request.getHeaderValue("Content-Length"));
	}
	else {
		session->fileSize = -1;
	}

	session->bind(&HTTP::recvFileFunc);
	enableReadEvent(request.getID());
	disableWriteEvent(request.getID());
}
