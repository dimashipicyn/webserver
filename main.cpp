#include <sstream>
#include "http.h"
#include "Request.h"
#include "Response.h"
#include "settingsManager/SettingsManager.hpp"

# define DEFAULT_CONFIG "settingsManager/webserv_default.yaml"

int main(int argc, char **argv)
{
	SettingsManager *settingsManager = SettingsManager::getInstance();
	try
	{
		settingsManager->parseConfig(argc == 2 ? argv[1] : DEFAULT_CONFIG);
	} catch (std::runtime_error &e) {
		settingsManager->clear();
		settingsManager->parseConfig(DEFAULT_CONFIG);
		std::cout << e.what() << std::endl;
		std::cout << "Applying default configuration" << std::endl;
	}

    HTTP serve("127.0.0.1", 1234);
    serve.start();
    return 0;
}
