//
// Created by Руслан Вахитов on 25.12.2021.
//

#ifndef WEBSERV_SETTINGSMANAGER_HPP
#define WEBSERV_SETTINGSMANAGER_HPP

#include <vector>
#include "Server.hpp"


class SettingsManager
{
protected:
	SettingsManager();

public:
	virtual ~SettingsManager();

protected:
	static SettingsManager *settingsManager_;

private:
	SettingsManager(SettingsManager const &copy);
	void operator=(SettingsManager const &);

	std::vector<Server *> servers_;
public:
	const std::vector<Server *> &getServers() const;

public:
	static SettingsManager *getInstance();
	void addServer(Server *server);
	Server *getLastServer();
};


#endif //WEBSERV_SETTINGSMANAGER_HPP
