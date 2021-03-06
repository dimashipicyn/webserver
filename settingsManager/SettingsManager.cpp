//
// Created by Руслан Вахитов on 25.12.2021.
//

#include "SettingsManager.hpp"
#include "Server.hpp"
#include "utils.h"

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
	validate();
}

void SettingsManager::clear()
{
	servers_.clear();
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

Server *SettingsManager::findServer(const std::string &hostFull)
{
	if (!utils::isValidPairString(hostFull, ':'))
		return nullptr;

	std::pair<std::string, std::string> map = utils::breakPair(hostFull, ':');
	return findServer(map.first, utils::to_number<uint16_t>(map.second));

}

void SettingsManager::validate()
{
	for (std::vector<Server>::iterator i = servers_.begin(); i != servers_.end(); i++) {
		std::vector<Server> duplicates;
		std::string host = (*i).getHost();
		uint16_t port = (*i).getPort();
		for (std::vector<Server>::iterator j = servers_.begin(); j != servers_.end(); j++) {

			if (host == (*j).getHost() && port == (*j).getPort())
				duplicates.push_back((*j));
		}
		if (duplicates.size() > 1)
			throw std::runtime_error("Config error: duplicates servers!");
	}
}
