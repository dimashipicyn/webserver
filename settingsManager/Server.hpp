//
// Created by Руслан Вахитов on 25.12.2021.
//

#ifndef WEBSERV_SERVER_HPP
#define WEBSERV_SERVER_HPP

#include <string>

#include "Route.hpp"

class Server
{
public:

	enum {
		KB = 1024,
		MB = 1024 * 1024,
		GB = 1024 * 1024 * 1024
	};

	Server();

	virtual ~Server();


	/**
	 * @brief Добавляет пустой роут в конец
	 */
	void addRoute();

	/**
	 * @brief Добавляет роут в конец
	 *
	 * @param указатель на роут
	 */
	void addRoute(Route &route);

	/**
	 * @brief достает последний роут из вектора
	 *
	 * @return крайный роут
	 */
	Route *getLastRoute();

	/**
	 * @brief валидация сервера на минимаьные требования
	 */
	bool isValid() const;

	/**
	 * @brief поиск наиболее подходящего роута по location
	 * @param path запрошенный ресурс
	 * @return указатель на найденнй роут или nullptr, если таковой не найден
	 */
	Route *findRouteByPath(std::string const &path);

	// геттеры сеттеры
	size_t getMaxBodySize() const;

	void setMaxBodySize(size_t maxBodySize);

	const std::string &getHost() const;

	void setHost(const std::string &host);

	uint16_t getPort() const;

	void setPort(short port);

	const std::string &getServerName() const;

	void setServerName(const std::string &serverName);

	const std::string &getErrorPage() const;

	void setErrorPage(const std::string &errorPage);

	const std::vector<Route> &getRoutes() const;

	void setRoutes(const std::vector<Route> &routes);

private:
	// хост dn или ip
	std::string host_;
	// порт
	uint16_t port_;
	// название сервера
	std::string serverName_;
	// абсолютный путь до дефолтной ошибки
	std::string errorPage_;
	// ограничение размера тела сообщения
	size_t maxBodySize_;
	// роуты
	std::vector<Route> routes_;
};


#endif //WEBSERV_SERVER_HPP
