//
// Created by Руслан Вахитов on 25.12.2021.
//

#include "ConfigParser.hpp"

class SettingsManager;

int main(int argc, char **argv) {
	if (argc != 2) {
		std::cout << "Ooops, you forgot to pass a config file!" << std::endl;
		return 1;
	}

	SettingsManager *settingsManager = SettingsManager::getInstance();
	ConfigParser *parser = new ConfigParser;
	parser->parseConfig(argv[1], settingsManager);

	std::cout << "Default server = " << settingsManager->getServers().front()->getHost() << ":" <<
		settingsManager->getServers().front()->getPort() << std::endl;
	std::cout << "Last server = " << settingsManager->getLastServer()->getHost() << ":" <<
			  settingsManager->getLastServer()->getPort() << std::endl;
}
