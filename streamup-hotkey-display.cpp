#include "streamup-hotkey-display.hpp"
#include "version.h"
#include <obs.h>
#include <obs-data.h>
#include <obs-encoder.h>
#include <obs-frontend-api.h>
#include <obs-module.h>
#include <QMainWindow>
#include <QDockWidget>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QFileInfo>
#include <util/platform.h>

#define QT_UTF8(str) QString::fromUtf8(str)
#define QT_TO_UTF8(str) str.toUtf8().constData()

OBS_DECLARE_MODULE()
OBS_MODULE_AUTHOR("Andilippi");
OBS_MODULE_USE_DEFAULT_LOCALE("streamup-hotkey-display", "en-US")

bool obs_module_load()
{
	blog(LOG_INFO, "[StreamUP Hotkey Display] loaded version %s", PROJECT_VERSION);


	return true;
}

void obs_module_post_load(void)
{

}

//--------------------EXIT COMMANDS--------------------
void obs_module_unload()
{
}

MODULE_EXPORT const char *obs_module_description(void)
{
	return obs_module_text("Description");
}

MODULE_EXPORT const char *obs_module_name(void)
{
	return obs_module_text("StreamUPHotkeyDisplay");
}
