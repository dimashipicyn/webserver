//
// Created by Lajuana Bespin on 11/15/21.
//

#ifndef WEBSERV_UTILS_H
#define WEBSERV_UTILS_H
#include <sstream>
#include <sys/stat.h>
#include <sys/time.h>
#include <fstream>
#include "httpExceptions.h"

namespace utils
{
    int get_next_line(int fd, std::string &line);

    std::vector<std::string> split(std::string &s, const char delim);

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

    /**
    * @brief Функция разбивает строку на ключ и значение.
    *
    * @param line - строка из файла конфигурации
    * @param delimiter - разделитель
    *
    * @return пара стрингов pair<ключ, значение>
    */
    std::pair<std::string, std::string> breakPair(std::string const &line, const char &delimiter);

    /**
     * @brief Функция извлекает расширение файла из запрошенного ресурса.
     *
     * @param resource - строка uri
     * @return строку формата ".xxx". Если в строке расширения не найдено - пустую строку
     */
    std::string getExtension(std::string const &resource);

	/**
	 * @brief правильно склеивает две части адреса, что бы между двумя кусками всегда был только один слэш
	 * @param first кусок начала
	 * @param second кусок в конец
	 * @return строка first + / + second
	 */
	std::string glueUri(const std::string &first, const std::string &second);

	/**
	 * @brief ищет в uri расширение сжи.
	 * @param resource - uri
	 * @param cgiExtension - расширение сжи
	 * @return позицию следующего за расширением символа в uri
	 */
	size_t checkCgiExtension(const std::string &resource, const std::string &cgiExtension);

	std::string getPathInfo(const std::string &resource, size_t cgiEndsAt);

    template<typename T>
    std::string to_string(const T &t)
    {
        std::stringstream ss;
        std::string result;

        ss << t;
        ss >> result;
        return result;
    }

    template<typename T>
    std::string to_hex_string(const T &t)
    {
        std::stringstream ss;
        std::string result;

        ss << std::hex << t;
        ss >> result;
        return result;
    }

    template<typename T>
    T to_number(const std::string &s)
    {
        std::stringstream ss;
        T result;

        ss << s;
        ss >> result;
        return result;
    }

    template<typename T>
    T to_hex_number(const std::string &s)
    {
        std::stringstream ss;
        T result;

        ss << s;
        ss >> std::hex >> result;
        return result;
    }

	bool isFile(const std::string& path);

	std::string		getDate(void);

	std::string readFile(const std::string &path);

	template< class InputIt,
			class OutputIt,
			class UnaryOperation >
	OutputIt transform( InputIt first1, InputIt last1, OutputIt d_first, UnaryOperation unary_op )
	{
		while (first1 != last1) {
			*d_first++ = unary_op(*first1++);
		}
		return d_first;
	}
}

#endif //WEBSERV_UTILS_H
