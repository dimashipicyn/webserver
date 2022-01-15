//
// Created by Руслан Вахитов on 25.12.2021.
//

#include <sstream>
#include "Server.hpp"
#include "utils.h"
#include "ConfigParser.hpp"
#include "SettingsManager.hpp"

ConfigParser::ConfigParser() : lineCounter_(0)
{}

void ConfigParser::parseConfig(const std::string &fileName)
{
	SettingsManager *settingsManager = SettingsManager::getInstance();
	std::string line;
	std::string previousLine;
	Server *currentServer;

	std::ifstream config(fileName);
	if (!config.is_open())
		throw std::runtime_error("Config error: Can not open the file!");

	while(true) {
		if (config.eof()) break;
		if (previousLine.empty())
		{
			getline(config, line);
			lineCounter_++;
		}
		else
		{
			line = previousLine;
			previousLine = "";
		}
		line = utils::trim(line, " \t");
		if (line[0] == '#' || line.empty())
			continue;

		if (line == "server:")
		{
			if (!settingsManager->getServers().empty() && !settingsManager->getServers().back().isValid())
				throw std::runtime_error(formConfigErrorText("Server block does not meet minimum requirements!"));
			settingsManager->addServer();
			continue;
		}
		else if (settingsManager->getServers().empty())
			throw std::runtime_error(formConfigErrorText("Should start with server block!"));

		if (!utils::isValidPairString(line, ':'))
			continue;

		std::pair<std::string, std::string> map = utils::breakPair(line, ':');
		currentServer = settingsManager->getLastServer();
		switch (parameterMapping(map.first))
		{
			case HOST:
				currentServer->setHost(map.second);
				break;
			case PORT:
				currentServer->setPort(atoi(map.second.c_str()));
				break;
			case SERVER_NAME:
				currentServer->setServerName(map.second);
				break;
			case ERROR_PAGE:
				currentServer->setErrorPage(map.second);
				break;
			case MAX_BODY_SIZE:
				currentServer->setMaxBodySize(parseBodySize(map.second));
				break;
			case ROUTE:
				previousLine = parseRoute(config, *currentServer);
				break;
			default:
				break;
		}
	}
}

ConfigParser::parameterCode ConfigParser::parameterMapping(const std::string &str)
{
	if (str == "host") return HOST;
	if (str == "port") return PORT;
	if (str == "server_name") return SERVER_NAME;
	if (str == "error_page") return ERROR_PAGE;
	if (str == "max_body_size") return MAX_BODY_SIZE;
	if (str == "route") return ROUTE;
	if (str == "location") return LOCATION;
	if (str == "root") return ROOT;
	if (str == "default_file") return DEFAULT_FILE;
	if (str == "upload_to") return UPLOAD_TO;
	if (str == "method") return METHOD;
	if (str == "redirect") return REDIRECT;
	if (str == "autoindex") return AUTOINDEX;
	if (str == "cgi") return CGI;
	return NOT_IMPLEMENTED;
}

size_t ConfigParser::parseBodySize(const std::string &str)
{
	char *end;
	size_t value = std::strtoul(str.c_str(), &end, 10);
	if (end[0] == 0 || end[1] != 0)
		throw std::runtime_error(formConfigErrorText("Invalid max_body_size parameter!"));
	switch (end[0])
	{
		case 'K':
			return value * Server::KB;
		case 'M':
			return value * Server::MB;
		case 'G':
			return value * Server::GB;
		default:
			throw std::runtime_error(formConfigErrorText("Invalid max_body_size sizing!"));
	}
}

std::string ConfigParser::parseRoute(std::ifstream &config, Server &server)
{
	std::string line;
	std::string previousLine;
	Route *currentRoute;

	while(true)
	{
		if (config.eof()) break;
		if (previousLine.empty())
		{
			getline(config, line);
			lineCounter_++;
		}
		else
		{
			line = previousLine;
			previousLine = "";
		}
		line = utils::trim(line, " \t");
		if (line[0] == '#' || line.empty())
			continue;

		if (line[0] == '-' && line[1] != '-') {
			line = utils::trim(line.substr(1), " \t");
			if (!server.getRoutes().empty() && !server.getRoutes().back().isValid())
				throw std::runtime_error(formConfigErrorText("Route block does not meet minimum requirements!"));
			server.addRoute();
		}
		else if (server.getRoutes().empty())
			throw std::runtime_error(formConfigErrorText("Routes not specified! Delete tag \"route\" or add element"));
		currentRoute = server.getLastRoute();
		std::pair<std::string, std::string> map = utils::breakPair(line, ':');
		switch (parameterMapping(map.first)) {
			case LOCATION:
				currentRoute->setLocation(map.second);
				break;
			case ROOT:
				currentRoute->setRoot(map.second);
				break;
			case DEFAULT_FILE:
				currentRoute->setDefaultFiles(std::vector<std::string>());
				while (true) {
					getLineAndTrim(config, line);
					if (line[0] == '-' && line[1] != '-')
					{
						line = utils::trim(line.substr(1), " \t");
						currentRoute->addDefaultFile(line);
					} else {
						previousLine = line;
						break;
					}
				}
				break;
			case UPLOAD_TO:
				currentRoute->setUploadTo(map.second);
				break;
			case METHOD:
				currentRoute->setMethods(std::vector<std::string>());
				while (true) {
					getLineAndTrim(config, line);
					if (line[0] == '-' && line[1] != '-')
					{
						line = utils::trim(line.substr(1), " \t");
						if (!currentRoute->isValidMethod(line))
							throw std::runtime_error(formConfigErrorText(
									"Invalid method \"" + line + "\"! Should be GET, POST or DELETE"));
						currentRoute->addMethod(line);
					} else {
						previousLine = line;
						break;
					}
				}
				break;
			case REDIRECT:
				while (true)
				{
					getLineAndTrim(config, line);
					if (line == "- rewrite:")
					{
						parseRedirect(config, *currentRoute);
					} else {
						previousLine = line;
						break;
					}
				}
				break;
			case AUTOINDEX:
				currentRoute->setAutoindex(map.second == "on");
				break;
			case CGI:
				currentRoute->setCgi(map.second);
				break;
			default:
				return line;
		}
	}
	return "";
}

void ConfigParser::getLineAndTrim(std::ifstream &config, std::string &line)
{
	getline(config, line);
	line = utils::trim(line, " \t");
	lineCounter_++;
}

void ConfigParser::parseRedirect(std::ifstream &config, Route &route)
{
	char *end;
	std::string line;
	Route::redirect r;

	r.status = 302;
	for (int i = 0; i < 3; i++) {
		getLineAndTrim(config, line);
		std::pair<std::string, std::string> map = utils::breakPair(line, ':');
		if (map.first == "from")
			r.from = map.second;
		else if (map.first == "to")
			r.to = map.second;
		else if (map.first == "status") {
			r.status = strtoul(map.second.c_str(), &end, 10);
			if (end == nullptr || *end != 0)
				throw std::runtime_error(formConfigErrorText("Invalid redirection status code"));
		}

	}
	if (r.from.empty() || r.to.empty() || (r.status < 301 || r.status > 302))
		throw std::runtime_error(formConfigErrorText("Invalid redirect parameter"));
	route.addRedirect(r);
}

std::string ConfigParser::formConfigErrorText(const std::string &message)
{
	std::ostringstream result;
	result << "Config error [line " << lineCounter_ << "]: " << message;
	return result.str();
}
