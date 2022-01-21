//
// Created by Руслан Вахитов on 28.12.2021.
//

#include "Route.hpp"
#include "utils.h"
#include "httpExceptions.h"
#include <unistd.h>

Route::Route()
{
	// defaults
	location_ = "/";
	root_ = "/var/www";
	defaultFiles_.push_back("index");
	defaultFiles_.push_back("index.html");
	defaultFiles_.push_back("index.htm");
	autoindex_ = false;
	cgi_ = ".cgi";
	uploadTo_ = "tmp";
}

Route::~Route()
{
	defaultFiles_.clear();
	methods_.clear();
	redirects_.clear();
}

bool Route::isValid() const
{
	bool result = true;

	if (location_.c_str() == nullptr || root_.c_str() == nullptr)
		result = false;

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

std::string Route::getFullPath(const std::string &resource) const
{
	std::string root = root_;
	std::string mergedResource = resource;
	std::string location = location_[location_.length() - 1] == '/' ? location_.substr(0, location_.length() - 1) :
			location_;
	size_t position = resource.find(location);
	if (position != std::string::npos) {
		mergedResource = mergedResource.erase(position, location.length());
	}
	return utils::glueUri(root, mergedResource);
}

std::string Route::getDefaultPage(const std::string &resource)
{
	std::string fullPath = getFullPath(resource);
	std::string line;
	std::string result = "";

	for (std::vector<std::string>::const_iterator i = defaultFiles_.begin(); i != defaultFiles_.end(); i++)
	{
		std::string tryPath = utils::glueUri(fullPath, (*i));
		if (access(tryPath.c_str(), F_OK) != -1) {
			std::ifstream file(tryPath);
			while (true)
			{
				if (file.eof()) break;
				getline(file, line);
				result += line;
			}
			return result;
		}
	}
	throw DefaultFileNotFoundException("There is no default files at directory");
}

int Route::checkRedirectOnPath(std::string &redirectTo, const std::string &resource)
{
	std::vector<std::string> splittedResource = utils::split(const_cast<std::string &>(resource), '/');
	Route::redirect *mostEqualRedirect = nullptr;
	int status = -1;
	uint32_t mostEqualLevel = 0;
	for (std::vector<Route::redirect>::iterator i = redirects_.begin(); i != redirects_.end(); i++)
	{
		std::string redirectWhat = utils::glueUri(location_, (*i).from);
		if (redirectWhat[redirectWhat.size() - 1] != '/')
		{
			if (redirectWhat == resource)
			{
				redirectTo = (*i).to;
				return (*i).status;
			}
			continue;
		}
		std::vector<std::string> splittedRedirect = utils::split(const_cast<std::string &> (redirectWhat), '/');
		uint32_t currentLevel = 0;
		size_t maxDepth = std::min(splittedResource.size(), splittedRedirect.size());
		if (maxDepth > mostEqualLevel)
		{
			for (size_t j = 0; j < maxDepth; j++)
			{
				if (splittedResource.at(j) == splittedRedirect.at(j))
					currentLevel++;
				else
					break;
			}
			if (currentLevel > mostEqualLevel)
			{
				mostEqualLevel = currentLevel;
				mostEqualRedirect = &(*i);
			}
		}
	}
	if (mostEqualRedirect != nullptr) {
		std::string tail = "";
		for (std::vector<std::string>::iterator i = splittedResource.begin() + mostEqualLevel;
			i != splittedResource.end();i++)
			tail += "/" + (*i);
		redirectTo = mostEqualRedirect->to + tail;
		status = mostEqualRedirect->status;
	}
	return status;
}

std::string Route::getDefaultFileName(const std::string &resource)
{
	std::string fullPath = getFullPath(resource);
	std::string line;
	std::string result = "";

	for (std::vector<std::string>::const_iterator i = defaultFiles_.begin(); i != defaultFiles_.end(); i++)
	{
		std::string tryPath = utils::glueUri(fullPath, (*i));
		if (utils::isFile(tryPath)) {
			return tryPath;
		}
	}
	throw DefaultFileNotFoundException("There is no default files at directory");
}
