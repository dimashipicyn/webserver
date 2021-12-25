//
// Created by Руслан Вахитов on 25.12.2021.
//

#include "SettingsManager.hpp"

SettingsManager *SettingsManager::settingsManager_ = 0;

SettingsManager::SettingsManager()
{

}

SettingsManager *SettingsManager::getInstance()
{
	if (settingsManager_ == NULL)
		settingsManager_ = new SettingsManager();
	return settingsManager_;
}

SettingsManager::~SettingsManager()
{

}

const std::vector<Server *> &SettingsManager::getServers() const
{
	return servers_;
}

void SettingsManager::addServer(Server *server)
{
	servers_.push_back(server);
}

Server *SettingsManager::getLastServer()
{
	return servers_.back();
}
