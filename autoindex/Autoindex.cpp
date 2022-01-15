//
// Created by Руслан Вахитов on 10.01.2022.
//

#include <dirent.h>
#include <sys/stat.h>
#include "httpExceptions.h"

#include "Autoindex.hpp"
# define FILENAME_LIMIT 50


Autoindex::Autoindex(Route const &route) : route_(route){}

std::string Autoindex::generatePage(const std::string &resource)
{
	std::string fullPath = route_.getRoot() + resource;
	std::string result = std::string("<html>\n")
						 + "<head><title>Index of "
						 + resource
						 + "</title></head>\n"
						 + "<body bgcolor=\"white\">\n"
						 + "<h1>Index of "
						 + resource
						 + "</h1><hr><pre><a href=\"../\">../</a>\n";
	struct stat info;
	struct dirent *ent;
	DIR *dir;
	if ((dir = opendir(fullPath.c_str())) != nullptr) {
		/* print all the files and directories within directory */
		while ((ent = readdir (dir)) != nullptr) {
			std::string tmp = ent->d_name;
			if (tmp == "." || (tmp[0] == '.' && tmp[1] != 0))
				continue;
			stat((fullPath + tmp).c_str(), &info);
			if (ent->d_type == DT_DIR)
				tmp += "/";
			result += std::string("<a href=\"") + tmp + "\">" + formatFilename(tmp)
					  + "</a>"
					  + putRemainSpaces(tmp)
					  + " "
					  + timespecToUtcString(info.st_mtimespec)
					  + "\t\t\t"
					  + (ent->d_type == DT_DIR ? "-" : sizeToString(info.st_size))
					  + "\n";
		}
		closedir (dir);
	} else {
		throw httpEx<NotFound>(std::string ("No such directory!") + " \"" + fullPath + "\"");
	}
	result += "</pre><hr></body>\n</html>\n";
	return result;
}

std::string Autoindex::timespecToUtcString(timespec const &lastModified)
{
	const int tmpsize = 18;
	char buf[tmpsize];
	buf[tmpsize - 1] = 0;
	struct tm tm;
	gmtime_r(&lastModified.tv_sec, &tm);
	strftime(buf, tmpsize, "%d-%b-%Y %H:%M", &tm);
	return buf;
}

std::string Autoindex::sizeToString(const off_t &fileSize)
{
	char buf[31];
	bzero(buf, 31);
	sprintf(buf, "%lld", fileSize);
	return buf;
}

std::string Autoindex::formatFilename(const std::string &filename)
{
	return filename.size() > FILENAME_LIMIT ? filename.substr(0, FILENAME_LIMIT - 3) + "..>" : filename;
}

std::string Autoindex::putRemainSpaces(const std::string &filename)
{
	size_t remain = FILENAME_LIMIT > filename.size() ? FILENAME_LIMIT - (filename.size() - 1) : 1;
	char result[remain];
	result[remain - 1] = 0;
	if (remain > 1)
	memset(result, ' ', remain - 1);
	return result;
}
