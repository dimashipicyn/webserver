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
	meta["SERVER_SOFTWARE"] = "webserv/1.0";


	size_t i = 0;
	for (std::map<std::string, std::string>::iterator iter = meta.begin(); iter != meta.end(); iter++) {
		std::string element = iter->first + "=" + iter->second;
		env_.push_back(new char[element.size() + 1]);
		strcpy(env_.at(i++), element.c_str());
	}
	env_.push_back(nullptr);
}

int Cgi::runCGI(int fd)
{
	pid_t pid;
	script_ = "./cgi_tester";

    int fds[2];
    pipe(fds);

    int from = fd;
    int to = fds[1];

    waitpid(-1, nullptr, 0);

	pid = fork();

	if (pid < 0) {
		LOG_ERROR("Internal server error: cannot fork!\n");
        throw httpEx<InternalServerError>("Fork error");
	}
	if (pid == 0) {
        dup2(from, STDIN_FILENO);
        close(from);

        dup2(to, STDOUT_FILENO);
        close(to);

        close(fds[0]);

        execve(script_.c_str(), nullptr, env_.data());
        exit(-1);
    } else {
        close(to);
    }

    return fds[0];
}

Cgi::~Cgi()
{
	for (std::vector<char *>::iterator i = env_.begin(); i != env_.end(); i++)
		delete[](*i);
	env_.clear();
}
