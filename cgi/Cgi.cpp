//
// Created by Руслан Вахитов on 03.01.2022.
//

#include <unistd.h>

#include "Cgi.hpp"
#include "Request.h"
#include "utils.h"
#include "Logger.h"
#include "Route.hpp"
#include "httpExceptions.h"
#include "SettingsManager.hpp"
#include "Server.hpp"

#define BUFFER 1024

Cgi::Cgi(const Request &request, const Route &route)
{
	script_ = route.getFullPath(request.getPath());
	body_ = request.getBody();
	convertMeta(request);
}

void Cgi::convertMeta(const Request &request)
{
	Request::headersMap meta;
	Request::headersMap headers = request.getHeaders();
	std::pair<std::string, std::string> host = utils::breakPair(request.getHost(), ':');
	std::string path = request.getPath();
	Route *route = SettingsManager::getInstance()->findServer(request.getHost())->findRouteByPath(path);
	size_t scriptEnd = utils::checkCgiExtension(path, route->getCgi());
	std::string pathInfo = utils::getPathInfo(path, scriptEnd);

	// Конвертируем заголовки запроса и др параметры в cgi мета переменные
	meta["AUTH_TYPE"] = headers.find("auth-scheme") == headers.end() ? "" : headers["auth-scheme"];
	meta["CONTENT_LENGTH"] = std::to_string(request.getBody().length());
	meta["CONTENT_TYPE"] = headers["Content-Type"];
	meta["GATEWAY_INTERFACE"] = "CGI/1.1";
	meta["PATH_INFO"] = pathInfo;
	meta["REQUEST_METHOD"] = request.getMethod();
	meta["PATH_TRANSLATED"] = path;
	meta["QUERY_STRING"] = request.getQueryString();
	meta["REMOTE_ADDR"] =  request.getHost();
//	meta["SCRIPT_NAME"] = path.substr(0, scriptEnd);
	meta["SERVER_NAME"] = headers.find("Hostname") == headers.end() ? host.first : headers["Hostname"];
	meta["SERVER_PORT"] = host.second;
	meta["SERVER_PROTOCOL"] = "HTTP/1.1";
	meta["SERVER_SOFTWARE"] = "HTTP/1.1";


	size_t i = 0;
	for (std::map<std::string, std::string>::iterator iter = meta.begin(); iter != meta.end(); iter++) {
		std::string element = iter->first + "=" + iter->second;
		env_.push_back(new char[element.size() + 1]);
		strcpy(env_.at(i++), element.c_str());
	}
	env_.push_back(nullptr);
}

std::string Cgi::runCGI()
{
	pid_t pid;
	script_ = "./cgi_tester";
	FILE *fileIn = tmpfile();
	FILE *fileOut = tmpfile();
	int cgiIn = fileno(fileIn);
	int cgiOut = fileno(fileOut);
	int status = 0;

	std::string result;

	write(cgiIn, body_.c_str(), body_.size());
	lseek(cgiIn, 0, SEEK_SET);

	pid = fork();

	if (pid < 0) {
		LOG_ERROR("Internal server error: cannot fork!\n");
		throw httpEx<InternalServerError>("Internal server error: cannot fork!");
	}
	if (pid == 0) {
		std::cout << script_.c_str() << std::endl;
		dup2(cgiIn, STDIN_FILENO);
		dup2(cgiOut, STDOUT_FILENO);

		if (execve(script_.c_str(), nullptr, env_.data()) < 0) {exit(1);}
		exit(0);
	} else {
		int ret = 1;
		char buf[BUFFER];

		waitpid(-1, &status, 0);
		lseek(cgiOut, 0, SEEK_SET);
		if (WIFEXITED(status) == 0)
			throw httpEx<InternalServerError>("Cannot execute cgi script");
		while (ret > 0) {
			bzero(buf, BUFFER);
			ret = read(cgiOut, buf, BUFFER - 1);
			result += buf;
		}
	}
	return result;
}

Cgi::~Cgi()
{
	for (std::vector<char *>::iterator i = env_.begin(); i != env_.end(); i++)
		delete[](*i);
	env_.clear();
}
