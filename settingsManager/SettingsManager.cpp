//
// Created by Руслан Вахитов on 25.12.2021.
//

#include "SettingsManager.hpp"
#include "../utils.h"

SettingsManager *SettingsManager::settingsManager_ = nullptr;

SettingsManager::SettingsManager()
{
}

SettingsManager *SettingsManager::getInstance()
{
	if (settingsManager_ == nullptr)
		settingsManager_ = new SettingsManager();
	return settingsManager_;
}

SettingsManager::~SettingsManager()
{

}

const std::vector<Server> &SettingsManager::getServers() const
{
	return servers_;
}

void SettingsManager::addServer(Server &server)
{
	servers_.push_back(server);
}

void SettingsManager::addServer() {
	servers_.push_back(Server());
}

Server *SettingsManager::getLastServer()
{
	return &servers_.back();
}

void SettingsManager::parseConfig(const std::string &fileName)
{
	configParser_.parseConfig(fileName);
}

void SettingsManager::clear()
{
	servers_.clear();
}

const SettingsManager::Host &SettingsManager::getHost() const
{
	return host_;
}

void SettingsManager::setHost(const std::string &host, const uint16_t &port)
{
	host_.host = host;
	host_.port = port;
}

Server *SettingsManager::findServer(const std::string &host, const uint16_t &port)
{
	for (std::vector<Server>::iterator iter = servers_.begin(); iter != servers_.end(); iter++) {
		if (iter->getHost() == host && iter->getPort() == port)
			return &(*iter);
	}
	return nullptr;
}

Server *SettingsManager::getDefaultServer()
{
	return &servers_.front();
}
