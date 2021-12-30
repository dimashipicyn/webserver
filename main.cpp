#include <sstream>
#include "http.h"
#include "Request.h"
#include "Response.h"
#include "settingsManager/SettingsManager.hpp"

int main(int argc, char **argv)
{
	SettingsManager *settingsManager = SettingsManager::getInstance();
	try
	{
		settingsManager->parseConfig(argc == 2 ? argv[1] : settingsManager->getDefaultConfig());
	} catch (std::runtime_error &e) {
		settingsManager->clear();
		settingsManager->parseConfig(settingsManager->getDefaultConfig());
		std::cout << e.what() << std::endl;
		std::cout << "Applying default configuration" << std::endl;
	}

    HTTP serve("127.0.0.1", 1234);
    serve.start();
    return 0;
}
