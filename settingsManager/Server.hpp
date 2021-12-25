//
// Created by Руслан Вахитов on 25.12.2021.
//

#ifndef WEBSERV_SERVER_HPP
#define WEBSERV_SERVER_HPP

#include <iostream>

class Server
{
private:
	std::string host_;
	uint16_t port_;
public:
	const std::string &getHost() const;

	void setHost(const std::string &host);

	short getPort() const;

	void setPort(short port);

	bool isValid();
};


#endif //WEBSERV_SERVER_HPP
