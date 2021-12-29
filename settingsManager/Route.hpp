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

	struct redirect {
		std::string from;
		std::string to;
		bool rewrite;
	};

	Route();

	virtual ~Route();

private:
	std::string location_;
	std::string root_;
	std::vector<std::string> defaultFiles_;
	std::string uploadTo_;
	std::vector<std::string> methods_;
	std::vector<redirect> redirects_;
	bool autoindex_;

private:
	std::string cgi_;
public:

	bool isValid();

	bool isValidMethod(std::string const &str);

	void addDefaultFile(std::string const &defaultFile);

	void addMethod(std::string const &method);

	void addRedirect(Route::redirect &redirect);

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
