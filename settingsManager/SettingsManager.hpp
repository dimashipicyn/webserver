//
// Created by Руслан Вахитов on 25.12.2021.
//

#ifndef WEBSERV_SETTINGSMANAGER_HPP
#define WEBSERV_SETTINGSMANAGER_HPP

#include <unistd.h>


class SettingsManager
{
protected:
	SettingsManager();
	static SettingsManager *settingsManager_;

private:
	SettingsManager(SettingsManager const &copy);
	void operator=(SettingsManager const &);

public:
	static SettingsManager *getInstance();

};


#endif //WEBSERV_SETTINGSMANAGER_HPP
