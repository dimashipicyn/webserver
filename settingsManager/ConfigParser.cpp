//
// Created by Руслан Вахитов on 25.12.2021.
//

#include "ConfigParser.hpp"

int ConfigParser::parseConfig(const std::string &fileName, SettingsManager *settingsManager)
{
	std::string line;
	Server *currentServer;

	std::ifstream config(fileName);
	if (!config.is_open())
		return -1;

	while(true) {
		if (config.eof()) break;
		getline(config, line);
		if (line[0] == '#')
			continue;

		if (strcmp("server:", trim(line, " \t").c_str()) == 0)
		{
			if (!settingsManager->getServers().empty() && !settingsManager->getServers().back()->isValid())
				return -1;
			settingsManager->addServer(new Server());
			continue;
		}
		else if (settingsManager->getServers().empty())
			return -1;

		std::pair<std::string, std::string> map = breakPair(line);
		currentServer = settingsManager->getLastServer();
		switch (parameterMapping(map.first))
		{
			case HOST:
				currentServer->setHost(map.second);
				break;
			case PORT:
				currentServer->setPort(atoi(map.second.c_str()));
				break;
			default:
				break;
		}
	}

	return 0;
}

std::pair<std::string, std::string> ConfigParser::breakPair(const std::string &line)
{
	std::pair<std::string, std::string> result;
	size_t delimiter = line.find_first_of(':');
	result.first = trim(line.substr(0, delimiter), " \t");
	result.second = trim(line.substr(delimiter + 1, line.length()), " \t");

	return result;
}

bool ConfigParser::isValidLine(const std::string &line)
{
	return line.find_first_of(':') != std::string::npos && line.find_first_of(':') == line.find_last_of(':');
}

ConfigParser::parameterCode ConfigParser::parameterMapping(const std::string &str)
{
	if (str == "host") return HOST;
	if (str == "port") return PORT;
	return NOT_IMPLEMENTED;
}
