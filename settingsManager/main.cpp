//
// Created by Руслан Вахитов on 25.12.2021.
//

#include "SettingsManager.hpp"

class SettingsManager;

int main(int argc, char **argv)
{

	SettingsManager *settingsManager = SettingsManager::getInstance();

	try
	{
		settingsManager->parseConfig(argc == 2 ? argv[1] : "settingsManager/webserv_default.yaml");
	} catch (std::runtime_error &e) {
		settingsManager->clear();
		settingsManager->parseConfig("settingsManager/webserv_default.yaml");
		std::cout << e.what() << std::endl;
		std::cout << "Applying default configuration" << std::endl;
	}

	std::cout << "Default server = " << settingsManager->getServers().front()->getHost() << ":" <<
			  settingsManager->getServers().front()->getPort() << std::endl;
	std::cout << "Last server = " << settingsManager->getLastServer()->getHost() << ":" <<
			  settingsManager->getLastServer()->getPort() << std::endl;
}
