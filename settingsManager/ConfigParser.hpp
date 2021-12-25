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
	 */
	bool isValidLine(std::string const &line);


public:
	/*
	 * @brief Функция парсинга конфигурационного файла
	 *
	 * @param fileName путь + имя файла
	 * @param settingsManager ссылка на сеттинг менеджер, куда будет парситься конфигурация
	 *
	 * @return 0 - если успех, -1 - если ошибка
	 */
	int parseConfig(std::string const &fileName, SettingsManager *settingsManager);
};


#endif //WEBSERV_CONFIGPARSER_HPP
