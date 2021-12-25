//
// Created by Руслан Вахитов on 25.12.2021.
//

#include "SettingsManager.hpp"

SettingsManager::SettingsManager()
{

}

SettingsManager *SettingsManager::getInstance()
{
	if (settingsManager_ == NULL)
		settingsManager_ = new SettingsManager();
	return settingsManager_;
}
