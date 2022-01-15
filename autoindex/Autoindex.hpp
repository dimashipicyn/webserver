//
// Created by Руслан Вахитов on 10.01.2022.
//

#ifndef WEBSERV_AUTOINDEX_HPP
#define WEBSERV_AUTOINDEX_HPP

#include <iostream>
#include <sstream>

#include "Route.hpp"

class Autoindex
{
public:
	Autoindex(Route const &route);

	/**
	 * @info Генерирует html страничку для листинга. Скрытые файлы и папки не отображаются. Отображается дата изменения
	 * и размер файла.
	 *
	 * @param resource uri
	 *
	 * @return возвращает text/html тело в виде строки
	 *
	 * @throws runtime_error если на сервере не существует переданного ресурса
	 */
	std::string generatePage(std::string const &resource);


private:
	Route route_;

	/**
	 * @brief конвертирует timespec в строку формата date time UTC
	 *
	 * @param lastModified структура секунды + наносекунды
	 *
	 * @return строка формата "dd-MMM-yy HH:mm"
	 */
	std::string timespecToUtcString(timespec const &lastModified);

	/**
	 * @brief конвертирует размер файла в строку
	 *
	 * @param fileSize - размер файла
	 *
	 * @return число строкой
	 */
	std::string sizeToString(off_t const &fileSize);

	/**
	 * @brief форматирует название файла/директории, в случае если длинна названия превышает FILENAME_LIMIT.
	 *
	 * @param filename имя файла
	 *
	 * @return если название > FILENAME_LIMIT, обрезает его и добавляет "..>" в конец. В обратном случае возвращает
	 * filename
	 */
	std::string formatFilename(std::string const &filename);

	/**
	 * @brief для названия файла короче FILENAME_LIMIT формирует строку с пробелами, кратной количеству недостающих
	 * до FILENAME_LIMIT символов
	 *
	 * @param filename имя файла
	 *
	 * @return строку с проблеами. Если filename > FILENAME_LIMIT то пустую строку
	 */
	std::string putRemainSpaces(std::string const &filename);
};


#endif //WEBSERV_AUTOINDEX_HPP
