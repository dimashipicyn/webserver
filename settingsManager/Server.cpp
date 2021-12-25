//
// Created by Руслан Вахитов on 25.12.2021.
//

#include "Server.hpp"

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

bool Server::isValid()
{
	bool result = true;

	if (host_.c_str() == NULL || port_ == NULL)
		result = false;

	return result;
}
