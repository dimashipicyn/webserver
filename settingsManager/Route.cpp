//
// Created by Руслан Вахитов on 28.12.2021.
//

#include "Route.hpp"

Route::Route()
{
	root_ = "/var/www";
}

Route::~Route()
{
	defaultFiles_.clear();
	methods_.clear();
	redirects_.clear();
}

bool Route::isValid()
{
	bool result = true;

	// добавить логику

	return result;
}

const std::string &Route::getLocation() const
{
	return location_;
}

void Route::setLocation(const std::string &location)
{
	location_ = location;
}

const std::string &Route::getRoot() const
{
	return root_;
}

void Route::setRoot(const std::string &root)
{
	root_ = root;
}

const std::vector<std::string> &Route::getDefaultFiles() const
{
	return defaultFiles_;
}

void Route::setDefaultFiles(const std::vector<std::string> &defaultFiles)
{
	defaultFiles_ = defaultFiles;
}

const std::string &Route::getUploadTo() const
{
	return uploadTo_;
}

void Route::setUploadTo(const std::string &uploadTo)
{
	uploadTo_ = uploadTo;
}

const std::vector<std::string> &Route::getMethods() const
{
	return methods_;
}

void Route::setMethods(const std::vector<std::string> &methods)
{
	methods_ = methods;
}

const std::vector<Route::redirect> &Route::getRedirects() const
{
	return redirects_;
}

void Route::setRedirects(const std::vector<redirect> &redirects)
{
	redirects_ = redirects;
}

const std::string &Route::getCgi() const
{
	return cgi_;
}

void Route::setCgi(const std::string &cgi)
{
	cgi_ = cgi;
}

bool Route::isValidMethod(const std::string &str)
{
	return strcmp("GET", str.c_str()) == 0 || strcmp("POST", str.c_str()) == 0 || strcmp("DELETE", str.c_str()) == 0;
}

void Route::addMethod(const std::string &method)
{
	methods_.push_back(method);
}

void Route::addDefaultFile(const std::string &defaultFile)
{
	defaultFiles_.push_back(defaultFile);
}

void Route::addRedirect(Route::redirect &redirect)
{
	redirects_.push_back(redirect);
}

bool Route::isAutoindex() const
{
	return autoindex_;
}

void Route::setAutoindex(bool autoindex)
{
	autoindex_ = autoindex;
}
