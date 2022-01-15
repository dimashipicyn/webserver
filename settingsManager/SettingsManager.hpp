//
// Created by Руслан Вахитов on 25.12.2021.
//

#ifndef WEBSERV_SETTINGSMANAGER_HPP
#define WEBSERV_SETTINGSMANAGER_HPP

#include <vector>
#include "ConfigParser.hpp"

/**
 * @brief Объект представления данных из конфигурационного файла. Синглтон
 */
class SettingsManager
{
public:

	// Информация о хосте
	struct Host
	{
		std::string host;
		uint16_t port;
	};

	virtual ~SettingsManager();

	const std::vector<Server> &getServers() const;

	static SettingsManager *getInstance();

	/**
	 * @brief добавляет сервер в конец
	 *
	 * @param server ссылка на сервер
	 */
	void addServer(Server &server);

	/**
	 * @brief добавляет пустой сервер
	 */
	void addServer();

	/**
	 * @brief Вызывает функцию парсинга класса <code>ConfigParser</code>
	 *
	 * @param fileName относительный путь до файла конфига
	 */
	void parseConfig(std::string const &fileName);

	/**
	 * @brief достает последний сервер из вектора без удаления
	 *
	 * @return указатель на крайний сервер
	 */
	Server *getLastServer();

	/**
	 * @brief достает дефолтный сервер из вектора без удаления. Сейчас это первый элемент.
	 *
	 * @return указатель на крайний сервер
	 */
	Server *getDefaultServer();

	/**
	 * @brief обнуляет вектор серверов с очисткой памяти
	 */
	void clear();

	/**
	 * @brief ищет сервер по хосту и порту
	 *
	 * @param host адрес хоста
	 * @param port порт хоста
	 *
	 * @return указатель на подходящий сервер в конфиге. nullptr если таков не найден.
	 */
	Server *findServer(std::string const &host, uint16_t const &port);

	/**
	 * @brief перегрузка для строки формата "хост:порт"
	 */
	Server *findServer(std::string const &hostFull);

protected:
	SettingsManager();

	static SettingsManager *settingsManager_;


private:

	// Запрещаем копирование и присваивание
	SettingsManager(SettingsManager const &copy);

	void operator=(SettingsManager const &);

	// сервера, первый дефолтный
	std::vector<Server> servers_;
	// парсер
	ConfigParser configParser_;

};


#endif //WEBSERV_SETTINGSMANAGER_HPP
