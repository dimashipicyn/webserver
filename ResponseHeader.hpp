#ifndef RESPONSEHEADER_HPP
#define RESPONSEHEADER_HPP

#include <set>

class ResponseHeader {
public:
	ResponseHeader(void);
	ResponseHeader(const ResponseHeader & src);
	~ResponseHeader(void);

	ResponseHeader & operator=(const ResponseHeader & src);

	// Setter functions
	void	setHeader(const std::string&, const std::string&);
	void			setAllow(std::set<std::string> methods);
	void			setAllow(const std::string& allow = "");
	void			setContentLocation(const std::string& path, int code);
	void			setContentType(std::string type, std::string path);
	void			setDate(void);
	void			setLastModified(const std::string& path);
	void			setLocation(int code, const std::string& redirect);
	void			setRetryAfter(int code, int sec);
	void			setWwwAuthenticate(int code);

	// Member functions
	std::string		getHeader(const Request& request);
	std::string		notAllowed(std::set<std::string> methods, const std::string& path, int code, const std::string& lang);
	std::string		writeHeader(void);
	void			setValues(size_t size, const std::string& path, int code, std::string type, const std::string& contentLocation, const std::string& lang);
	std::string		getStatusMessage(int code);

private:
	static std::map<std::string, std::string> _headers;
	static std::map<std::string, std::string> resetHeaders();

	static const std::map<int, std::string>	_errors;
	static std::map<int, std::string>	initErrorMap();
};


#endif //WEBSERV_RESPONSEHEADER_HPP
