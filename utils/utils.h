//
// Created by Lajuana Bespin on 11/15/21.
//

#ifndef WEBSERV_UTILS_H
#define WEBSERV_UTILS_H
#include <sstream>
#include <sys/stat.h>

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
    T to_number(const std::string &s)
    {
        std::stringstream ss;
        T result;

        ss << s;
        ss >> result;
        return result;
    }

	bool isFile(const std::string& path);
}

#endif //WEBSERV_UTILS_H
