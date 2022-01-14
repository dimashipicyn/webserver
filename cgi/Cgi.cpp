//
// Created by Руслан Вахитов on 03.01.2022.
//

#include <unistd.h>

#include "Cgi.hpp"
#include "Request.h"
#include "SettingsManager.hpp"
#include "Logger.h"

#define BUFFER 1024

Cgi::Cgi(const Request &request)
{
	script_ = request.getPath(); // !!Поменять на абсолютный путь через функцию Route::getFullPath() когда будет ip:port
	body_ = request.getBody();
	convertMeta(request);
}

void Cgi::convertMeta(const Request &request)
{
	Request::headersMap meta;
	Request::headersMap headers = request.getHeaders();
	SettingsManager *settingsManager = SettingsManager::getInstance();

	// Конвертируем заголовки запроса и др параметры в cgi мета переменные
	meta["AUTH_TYPE"] = headers.find("auth-scheme") == headers.end() ? "" : headers["auth-scheme"];
	meta["CONTENT_LENGTH"] = std::to_string(request.getBody().length());
	meta["CONTENT_TYPE"] = headers["Content-Type"];
	meta["GATEWAY_INTERFACE"] = "CGI/1.1";
	meta["PATH_INFO"] = request.getPath();
	meta["PATH_TRANSLATED"] = request.getPath();
	meta["QUERY_STRING"] = request.getQueryString();
	meta["REMOTE_ADDR"] = settingsManager->getHost().host;

	// Возможно понадобится
//	meta["REMOTE_HOST"] = "";
//	meta["REMOTE_IDENT"] = "";

	// Если запрос с авторизацией, эта переменная должна быть установлена. Подставить userId
//	if (!meta_["AUTH_TYPE"].empty()) meta_["REMOTE_USER"] = "USER_ID";

	meta["REQUEST_METHOD"] = request.getMethod();
	meta["SCRIPT_NAME"] = request.getPath();
	meta["SERVER_NAME"] = headers.find("Hostname") == headers.end() ? settingsManager->getHost().host : headers["Hostname"];
	meta["SERVER_PORT"] = std::to_string(settingsManager->getHost().port);
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

std::string Cgi::runCGI()
{
	pid_t pid;
	FILE *fileIn = tmpfile();
	FILE *fileOut = tmpfile();
	int cgiIn = fileno(fileIn);
	int cgiOut = fileno(fileOut);
	std::string errorInternal = "Status: 500 Internal Server Error\r\n\r\n";

	std::string result;

	write(cgiIn, body_.c_str(), body_.size());
	lseek(cgiIn, 0, SEEK_SET);

	pid = fork();

	if (pid < 0) {
		LOG_ERROR("Internal server error: cannot fork!\n");
		return errorInternal;
	}
	if (pid == 0) {
		dup2(cgiIn, STDIN_FILENO);
		dup2(cgiOut, STDOUT_FILENO);

		if (execve(("." + script_).c_str(), nullptr, env_.data()) < 0)
//		if (execve("./cgi_tester", nullptr, env_) < 0)
		{
			LOG_ERROR("Execve fail!\n");
			write(STDOUT_FILENO, errorInternal.c_str(), errorInternal.size());
			exit(1);
		}
		exit(0);
	} else {
		int ret = 1;
		char buf[BUFFER];

		waitpid(-1, nullptr, 0);
		lseek(cgiOut, 0, SEEK_SET);

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
