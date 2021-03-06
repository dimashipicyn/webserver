//
// Created by Lajuana Bespin on 11/15/21.
//

#include <unistd.h>
#include <string>
#include <vector>
#include "utils.h"

namespace utils {

#define BUFFER_SIZE 1024

    int	get_next_line(int fd, std::string& line)
    {
        char		        buf[BUFFER_SIZE];
        static std::string  fds[1025];
        int			        n;

        if ( fd < 0 )
            return (-1);
        n = 0;
        std::string::iterator found = fds[fd].end();
        while ( (n = read(fd, buf, BUFFER_SIZE)) > 0 )
        {
            buf[n] = '\0';
            fds[fd].append(buf);
            found = std::find(fds[fd].begin(), fds[fd].end(), '\n');
            if ( found != fds[fd].end() )
                break ;
        }
        line = std::string(fds[fd].begin(), found);
        fds[fd] = std::string(found, fds[fd].end());
        if ( n < 0 || line.empty() )
            return (-1);
        if ( !fds[fd].empty() && n == 0 )
            return (0);
        return (1);
    }

    std::vector<std::string>    split(std::string& s, const char delim)
    {
        std::vector<std::string>    words;
        std::string::iterator       it = s.begin();
        std::string::iterator       last = s.end();

        while ( (last = std::find(it, s.end(), delim)) != s.end() ) {
            words.push_back(std::string(it, last));
            it = last + 1;
        }
        words.push_back(std::string(it, last));
        return words;
    }

    std::string trim(const std::string &str, const std::string &whitespace)
    {
        const size_t strBegin = str.find_first_not_of(whitespace);
        if (strBegin == std::string::npos)
            return "";

        const size_t strEnd = str.find_last_not_of(whitespace);
        const size_t strRange = strEnd - strBegin + 1;

        return str.substr(strBegin, strRange);
    }

    bool isValidPairString(const std::string &line, const char &delimiter)
    {
        return line.find_first_of(delimiter) != std::string::npos
        && line.find_first_of(delimiter) == line.find_last_of(delimiter);
    }

    std::string getExtension(const std::string &resource)
    {
        size_t position = resource.find_last_of('.', std::string::npos);
        return position == std::string::npos ? "" : resource.substr(position);
    }

	std::pair<std::string, std::string> breakPair(const std::string &line, const char &delimiter)
	{
		std::pair<std::string, std::string> result;
		size_t delimiterPosition = line.find_first_of(delimiter);
        result.first = utils::trim(line.substr(0, delimiterPosition), " \t\r\n");
        result.second = utils::trim(line.substr(delimiterPosition + 1, line.length()), " \t\r\n");

		return result;
	}
}

	bool utils::isFile(const std::string& path){
		struct stat s;
		if	(stat(path.c_str(), &s) == 0){
			if(s.st_mode & S_IFREG)
				return true;
		}
		return false;
	}

	std::string			utils::getDate(void){
		char			buffer[100];
		struct timeval	tv;
		struct tm		*tm;

		gettimeofday(&tv, NULL);
		tm = gmtime(&tv.tv_sec);
		strftime(buffer, 100, "%a, %d %b %Y %H:%M:%S GMT", tm);
		return std::string(buffer);
	}

	std::string utils::readFile(const std::string &path) {
		std::ifstream sourceFile;
		sourceFile.open(path.c_str(), std::ifstream::in);
		std::ostringstream os;
		if (!sourceFile.is_open()) throw httpEx<NotFound>("Cannot open requested resource");
		os << sourceFile.rdbuf();
		sourceFile.close();
		return os.str();
	}

std::string utils::glueUri(const std::string &first, const std::string &second)
{
	return (first[0] == '/' ? "" : "/")
			+ (first[first.length() - 1] == '/' ? first.substr(0, first.length() - 1) : first)
			+ (second[0] == '/' ? second : ("/" + second));
}

size_t utils::checkCgiExtension(const std::string &resource, const std::string &cgiExtension)
{
	size_t extensionAt = resource.find(cgiExtension);
	return extensionAt == std::string::npos ? std::string::npos : extensionAt + cgiExtension.size();
}

std::string utils::getPathInfo(const std::string &resource, size_t cgiEndsAt)
{
	std::string result = resource.substr(cgiEndsAt, resource.length());
	return result.empty() ? "/" : result;
}

/*
int main() {
    std::string s = "hello world bro";
    std::vector<std::string> w = split(s, ' ');

    for (auto t : w) {
        std::cout << t << std::endl;
    }
}
 */
