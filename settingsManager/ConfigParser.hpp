//
// Created by Руслан Вахитов on 25.12.2021.
//

#ifndef WEBSERV_CONFIGPARSER_HPP
#define WEBSERV_CONFIGPARSER_HPP

#include "SettingsManager.hpp"
#include "../utils.h"
#include <fstream>

class ConfigParser
{

private:

	enum parameterCode {
		HOST,
		PORT,
		SERVER_NAME,
		ERROR_PAGE,
		MAX_BODY_SIZE,
		ROUTE,
		LOCATION,
		ROOT,
		DEFAULT_FILE,
		METHOD,
		UPLOAD_TO,
		REDIRECT,
		AUTOINDEX,
		CGI,
		NOT_IMPLEMENTED
	};

	parameterCode parameterMapping(std::string const &str);
	/*
	 * @brief Функция разбивает строку на ключ и значение. Разделитель ':'
	 *
	 * @param line - строка из файла конфигурации
	 *
	 * @return пара стрингов pair<ключ, значение>
	 */
	std::pair<std::string, std::string> breakPair(std::string const &line);

	/*
	 * @brief Функция проверяет строку на шаблон соответствию шаблону "ключ: значение"
	 *
	 * @param line - строка из файла конфигурации
	 *
	 * @return true если строка соответствует формату
	 */
	bool isValidLine(std::string const &line);

	/*
	 * @brief парсит строку в размер тела сообщения
	 *
	 * @param str строка, представляющая размер
	 *
	 * @return полученный размер в байтах
	 */
	size_t parseBodySize(std::string const &str);

	/*
	 * @brief парсит блок route
	 *
	 * @param config ссылка на открытый конфиг файл
	 * @param server ссылка на сервер в который парсится роут
	 *
	 * @return первую строку, не подходящую под шаблон route. Если eof - null.
	 */
	std::string parseRoute(std::ifstream &config, Server &server);

	void parseRewrite(std::ifstream &config, Route &route);

	void getLineAndTrim(std::ifstream &config, std::string &line);


public:
	/*
	 * @brief Функция парсинга конфигурационного файла
	 *
	 * @param fileName путь + имя файла
	 * @param settingsManager ссылка на сеттинг менеджер, куда будет парситься конфигурация
	 *
	 * @throws runtimeException с сообщением об ошибке
	 */
	void parseConfig(std::string const &fileName, SettingsManager *settingsManager);
};


#endif //WEBSERV_CONFIGPARSER_HPP
