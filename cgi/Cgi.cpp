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

Cgi::Cgi(const Request &request, const Route &route) : body_(const_cast<std::string &>(request.getBody()))
{
	script_ = route.getFullPath(request.getPath());
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
	meta["PATH_INFO"] = path;
	meta["REQUEST_METHOD"] = request.getMethod();
	meta["PATH_TRANSLATED"] = path;
	meta["QUERY_STRING"] = request.getQueryString();
	meta["REMOTE_ADDR"] =  request.getHost();
//	meta["SCRIPT_NAME"] = path.substr(0, scriptEnd);
	meta["SERVER_NAME"] = headers.find("Hostname") == headers.end() ? host.first : headers["Hostname"];
	meta["SERVER_PORT"] = host.second;
	meta["SERVER_PROTOCOL"] = "HTTP/1.1";
	meta["SERVER_SOFTWARE"] = "webserv/1.0";

	for (Request::headersMap::const_iterator i = request.getHeaders().begin(); i != request.getHeaders().end(); i++) {
		std::string key = (*i).first;
		utils::transform(key.begin(), key.end(),key.begin(), ::toupper);
		meta[std::string("HTTP_") + key] = (*i).second;
	}

	size_t i = 0;
	for (std::map<std::string, std::string>::iterator iter = meta.begin(); iter != meta.end(); iter++) {
		std::string element = iter->first + "=" + iter->second;
		env_.push_back(new char[element.size() + 1]);
		strcpy(env_.at(i++), element.c_str());
	}
	env_.push_back(nullptr);
}

#include <fcntl.h>

std::string Cgi::runCGI()
{
    script_ = "./cgi_tester";
    pid_t pid;

    int in[2];
    int out[2];

    pipe(in);
    pipe(out);

    std::string errorInternal = "Status: 500 Internal Server Error\r\n\r\n";

    std::string result;

    pid = fork();

    if (pid < 0) {
        LOG_ERROR("Internal server error: cannot fork!\n");
        return errorInternal;
    }
    if (pid == 0) {
        dup2(in[0], STDIN_FILENO);
        //close(in[0]);
        close(in[1]);

        dup2(out[1], STDOUT_FILENO);
        //close(out[0]);
        //close(out[1]);

        if (execve(script_.c_str(), nullptr, env_.data()) < 0)
        {
            LOG_ERROR("Execve fail!\n");
            write(STDOUT_FILENO, errorInternal.c_str(), errorInternal.size());
            exit(1);
        }
        exit(0);
    } else {

        close(in[0]);
        close(out[1]);

        std::string buf;
        buf.resize(65536);

        ssize_t sizew = 1;
        ssize_t sizer = 1;
        while (sizew > 0 || sizer > 0) {
            sizew = write(in[1], body_.c_str(), std::min<size_t>(body_.size(), 65536));
            body_.erase(0, sizew);

            if (sizew == 0) {
                close(in[1]);
            }

            sizer = read(out[0], &buf[0], 65536);
            result.append(buf, 0, sizer);
        }



        waitpid(-1, nullptr, 0);

        close(out[0]);
    }

    return result;
}

Cgi::~Cgi()
{
	for (std::vector<char *>::iterator i = env_.begin(); i != env_.end(); i++)
		delete[](*i);
	env_.clear();
}
