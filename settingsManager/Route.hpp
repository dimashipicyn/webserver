//
// Created by Руслан Вахитов on 28.12.2021.
//

#ifndef WEBSERV_ROUTE_HPP
#define WEBSERV_ROUTE_HPP

#include <iostream>
#include <vector>
#include <unistd.h>
#include <fstream>
#include "httpExceptions.h"

class Route
{
public:
	/**
	 * Структура редиректа.
	 *
	 * @param form uri который нужно перенаправить
	 * @param to http адрес на которой редиректим
	 * @param status статус код для редиректа. 301 - постоянный, 302 - временный
	 */
	struct redirect {
		std::string from;
		std::string to;
		uint16_t status;
	};

	Route();

	virtual ~Route();

	class DefaultFileNotFoundException : public httpEx<NotFound> {
	public:
		explicit DefaultFileNotFoundException(const std::string &err) : httpEx(err)
		{}

	private:
		const char *what() const throw()
		{
			return httpEx::what();
		}
	};

private:
	// префикс ресурса
	std::string location_;
	// путь до ресурса
	std::string root_;
	// дефолтные файлы, которые нужно искать, если путь запроса это папка
	std::vector<std::string> defaultFiles_;
	// куда загружать файлы
	std::string uploadTo_;
	// методы
	std::vector<std::string> methods_;
	// HTTP редиректы
	std::vector<redirect> redirects_;
	// листинг вкл/выкл
	bool autoindex_;
	// расширение по которому будет работать cgi
	std::string cgi_;
public:

	/**
	 * @brief валидация Ruote
	 */
	bool isValid() const;

	/**
	 * @brief валидация http методов
	 *
	 * @param str строка для проверки
	 */
	static bool isValidMethod(std::string const &str);

	/**
	 * @brief добавляет элемент к файлам по умолчанию
	 *
	 * @param defaultFile относительный путь к файлу
	 */
	void addDefaultFile(std::string const &defaultFile);

	/**
	 * @brief добавляет элемент к методам
	 *
	 * @param method название метода
	 */
	void addMethod(std::string const &method);

	/**
	 * @brief добавляет элемент к редиректам
	 *
	 * @param redirect ссылка на структуру redirect
	 */
	void addRedirect(Route::redirect &redirect);

	/**
	 * @brief склеивает root + resource
	 *
	 * @param resource - запрошенный ресур, например "index.html".
	 *
	 * @return возрващает строку абсолютного пути до ресурс, без проверок
	 */
	std::string getFullPath(std::string const &resource) const;

	/**
	 * @info ищет по указанному ресурсу дефолтные файлы из текущего роута. Считывает в строку первый попавшийся файл.
	 * Бросает эксепшн, если не нашлось или такого ресурса не существует.
	 *
	 * @param resource - запрошенный ресур (директория)
	 *
	 * @return строку с содержимым из дефолтного файла.
	 *
	 * @throws DefaultFileNotFoundException если ни одного дефолтного файла не было найдено.
	 * Эсепшн наследуется от httpEx<NotFound>, в общем случае можно ловить родителя. Частный случай нужен для автоиндекса
	 *
	 * @throws httpEx<NotFound> если подается не существующий ресурс
	 */
	std::string getDefaultPage(std::string const &resource);

	/**
	 * @brief проверяет ресурс на редиректы
	 *
	 * @param redirectTo ссылка на строку куда писать адрес редиректа, если найден.
	 *
	 * @param resource ресурс из http запроса
	 *
	 * @return статус редиректа, -1 если редиректов по данному ресурсу не найдено
	 */
	int checkRedirectOnPath(std::string &redirectTo, std::string const &resource);

	// геттеры сеттеры
	const std::string &getLocation() const;

	void setLocation(const std::string &location);

	const std::string &getRoot() const;

	void setRoot(const std::string &root);

	const std::vector<std::string> &getDefaultFiles() const;

	void setDefaultFiles(const std::vector<std::string> &defaultFiles);

	const std::string &getUploadTo() const;

	void setUploadTo(const std::string &uploadTo);

	const std::vector<std::string> &getMethods() const;

	void setMethods(const std::vector<std::string> &methods);

	const std::vector<redirect> &getRedirects() const;

	void setRedirects(const std::vector<redirect> &redirects);

	const std::string &getCgi() const;

	void setCgi(const std::string &cgi);

	bool isAutoindex() const;

	void setAutoindex(bool autoindex);

};


#endif //WEBSERV_ROUTE_HPP
