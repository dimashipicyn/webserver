//
// Created by Lajuana Bespin on 11/15/21.
//

#ifndef WEBSERV_UTILS_H
#define WEBSERV_UTILS_H

int	get_next_line(int fd, std::string& line);
std::vector<std::string>    split(std::string& s, const char delim);
std::string trim(const std::string &str, const std::string &whitespace);

/**
	 * @brief Функция проверяет строку на соответствие шаблону "key <delimiter> value". Пример строка "foo: bar"
	 * с разделителем ':' вернет true
	 *
	 * @param line - строка из файла конфигурации
	 * @param delimiter - разделитель
	 *
	 * @return true если строка соответствует формату
	 */

bool isValidPairString(std::string const &line, const char &delimiter);

#endif //WEBSERV_UTILS_H
