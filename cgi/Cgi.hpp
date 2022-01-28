//
// Created by Руслан Вахитов on 03.01.2022.
//

#ifndef WEBSERV_CGI_HPP
#define WEBSERV_CGI_HPP

#include <iostream>
#include <map>
#include <vector>

class Request;
class Response;
class Route;

class Cgi
{
public:
	Cgi(Request const &request, Route const &route);

	virtual ~Cgi();

    std::string runCGI();
private:
	std::string script_;
	std::string &body_;
	std::vector<char *> env_;

	/**
	 * Конвертирует данные из запроса, конфига в мета переменные CGI
	 *
	 * @param request ссылка на запрос
	 */
	void convertMeta(Request const &request);

};


#endif //WEBSERV_CGI_HPP
