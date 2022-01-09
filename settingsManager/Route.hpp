//
// Created by Руслан Вахитов on 28.12.2021.
//

#ifndef WEBSERV_ROUTE_HPP
#define WEBSERV_ROUTE_HPP

#include <iostream>
#include <vector>

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
	 * @brief генерирует полный путь до ресурса на сервере. root + resource
	 *
	 * @param resource - запрошенный ресур, например "index.html". Если resource папка, будут производиться попытки
	 * найти дефолтные ресурсы поочередно.
	 *
	 * @return возрващает полный путь на сервере до ресурса. Если ничего не найдено - вернет пустую строку.
	 */
	std::string getFullPath(std::string const &resource);

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
