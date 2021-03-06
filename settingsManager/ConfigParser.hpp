//
// Created by Руслан Вахитов on 25.12.2021.
//

#ifndef WEBSERV_CONFIGPARSER_HPP
#define WEBSERV_CONFIGPARSER_HPP

#include <string>

class SettingsManager;
class Server;
class Route;

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

	size_t lineCounter_;

public:
	ConfigParser();

private:

	/**
	 * @brief вспомогательная функция маппинга строки к enum. Для удобства обработки switch case'ом
	 *
	 * @param str - строка которую нужно смапить
	 *
	 * @return enum элемент parameterCode. Если строка не подошла, возвращается NOT_IMPLEMENTED
	 */
	parameterCode parameterMapping(std::string const &str);

	/**
	 * @brief парсит строку в размер тела сообщения
	 *
	 * @param str строка, представляющая размер
	 *
	 * @return полученный размер в байтах
	 */
	size_t parseBodySize(std::string const &str);

	/**
	 * @brief парсит блок route
	 *
	 * @param config ссылка на открытый конфиг файл
	 * @param server ссылка на сервер в который парсится роут
	 *
	 * @return первую строку, не подходящую под шаблон route. Если eof - null.
	 */
	std::string parseRoute(std::ifstream &config, Server &server);

	/**
	 * @brief парсит редиректы
	 *
	 * @param config ссылка на открытый конфиг файл
	 * @param route ссылка на роут в который парсятся редиректы
	 */
	void parseRedirect(std::ifstream &config, Route &route);

	/**
	 * @brief считывает строку с ifstream и обрубает пробелы и табы.
	 *
	 * @param config ссылка на открытый конфиг файл
	 * @param в line присвоится новая обработанная строка
	 */
	void getLineAndTrim(std::ifstream &config, std::string &line);
	/**
	 * @brief формирует текст ошибки с указанием номера строки
	 * @param message текст ошибки
	 */
	std::string formConfigErrorText(std::string const &message);


public:
	/**
	 * @brief Функция парсинга конфигурационного файла
	 *
	 * @param fileName путь + имя файла
	 * @param settingsManager ссылка на сеттинг менеджер, куда будет парситься конфигурация
	 *
	 * @throws runtimeException с сообщением об ошибке
	 */
	void parseConfig(std::string const &fileName);
};


#endif //WEBSERV_CONFIGPARSER_HPP
