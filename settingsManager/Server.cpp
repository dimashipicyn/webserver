//
// Created by Руслан Вахитов on 25.12.2021.
//

#include "Server.hpp"

Server::Server()
{
	maxBodySize_ = 1 * MB;
}

const std::string &Server::getHost() const
{
	return host_;
}

void Server::setHost(const std::string &host)
{
	host_ = host;
}

short Server::getPort() const
{
	return port_;
}

void Server::setPort(short port)
{
	port_ = port;
}

const std::string &Server::getServerName() const
{
	return serverName_;
}

void Server::setServerName(const std::string &serverName)
{
	serverName_ = serverName;
}

const std::string &Server::getErrorPage() const
{
	return errorPage_;
}

void Server::setErrorPage(const std::string &errorPage)
{
	errorPage_ = errorPage;
}

size_t Server::getMaxBodySize() const
{
	return maxBodySize_;
}

void Server::setMaxBodySize(size_t maxBodySize)
{
	maxBodySize_ = maxBodySize;
}

const std::vector<Route *> &Server::getRoutes() const
{
	return routes_;
}

void Server::setRoutes(const std::vector<Route *> &routes)
{
	routes_ = routes;
}

bool Server::isValid()
{
	bool result = true;

	if (host_.c_str() == NULL || port_ == NULL)
		result = false;

	return result;
}

void Server::addRoute(Route *route)
{
	routes_.push_back(route);
}

Route *Server::getLastRoute()
{
	return routes_.back();
}
