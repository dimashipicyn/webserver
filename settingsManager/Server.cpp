//
// Created by Руслан Вахитов on 25.12.2021.
//

#include "utils.h"
#include "Server.hpp"

Server::Server() : maxBodySize_(1 * MB)
{}

const std::string &Server::getHost() const
{
	return host_;
}

void Server::setHost(const std::string &host)
{
	host_ = host;
}

uint16_t Server::getPort() const
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

const std::vector<Route> &Server::getRoutes() const
{
	return routes_;
}

void Server::setRoutes(const std::vector<Route> &routes)
{
	routes_ = routes;
}

bool Server::isValid() const
{
	bool result = true;

	if (host_.c_str() == nullptr || port_ == 0)
		result = false;

	return result;
}

void Server::addRoute()
{
	routes_.push_back(Route());
}

void Server::addRoute(Route &route)
{
	routes_.push_back(route);
}

Route *Server::getLastRoute()
{
	return &routes_.back();
}

Server::~Server()
{
	routes_.clear();
}

Route *Server::findRouteByPath(std::string const &path)
{
	std::vector<std::string> splittedPath = utils::split(const_cast<std::string &>(path), '/');
	Route *mostEqualRoute = nullptr;
	uint32_t mostEqualLevel = 0;
	for (std::vector<Route>::iterator iter = routes_.begin(); iter != routes_.end(); iter++) {
		std::string location = (*iter).getLocation();
		if (location[location.size() - 1] != '/'){
			if (location == path) {
				return &(*iter);
			}
			continue;
		}
		std::vector<std::string> splittedLocation = utils::split(const_cast<std::string &> (location), '/');
		uint32_t currentLevel = 0;
		size_t maxDepth = std::min(splittedPath.size(), splittedLocation.size());
		if (maxDepth > mostEqualLevel)
		{
			for (size_t i = 0; i < maxDepth; i++)
			{
				if (splittedPath.at(i) == splittedLocation.at(i))
					currentLevel++;
				else
					break;
			}
			if (currentLevel > mostEqualLevel) {
				mostEqualLevel = currentLevel;
				mostEqualRoute = &(*iter);
			}
		}
	}
	return mostEqualRoute;
}
