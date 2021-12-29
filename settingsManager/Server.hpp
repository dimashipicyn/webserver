//
// Created by Руслан Вахитов on 25.12.2021.
//

#ifndef WEBSERV_SERVER_HPP
#define WEBSERV_SERVER_HPP

#include "Route.hpp"

class Server
{
public:

	enum {
		KB = 1024,
		MB = 1024 * 1024,
		GB = 1024 * 1024 * 1024
	};

	Server();

	size_t getMaxBodySize() const;

	void setMaxBodySize(size_t maxBodySize);

	void addRoute(Route *route);

	Route *getLastRoute();

	const std::string &getHost() const;

	void setHost(const std::string &host);

	short getPort() const;

	void setPort(short port);

	bool isValid();

	const std::string &getServerName() const;

	void setServerName(const std::string &serverName);

	const std::string &getErrorPage() const;

	void setErrorPage(const std::string &errorPage);

private:
	std::string host_;
	uint16_t port_;
	std::string serverName_;
	std::string errorPage_;
	size_t maxBodySize_;
	std::vector<Route *> routes_;
public:
	const std::vector<Route *> &getRoutes() const;

	void setRoutes(const std::vector<Route *> &routes);
};


#endif //WEBSERV_SERVER_HPP
