//
// Created by Руслан Вахитов on 03.01.2022.
//

#ifndef WEBSERV_CGI_HPP
#define WEBSERV_CGI_HPP

#include <iostream>
#include <map>
#include <unistd.h>
#include "../Request.h"
#include "../settingsManager/SettingsManager.hpp"

class Cgi
{
public:
	Cgi(Request const &request);

	virtual ~Cgi();

	std::string runCGI();
private:
	std::string script_;
	std::string body_;
	char **env_;

	/**
	 * Конвертирует данные из запроса, конфига в мета переменные CGI
	 *
	 * @param request ссылка на запрос
	 * @return переменные окружения cgi в виде char **env
	 */
	char **convertMeta(Request const &request);

};


#endif //WEBSERV_CGI_HPP
