#include "streamup-hotkey-display-settings.hpp"
#include <obs-frontend-api.h>

extern obs_data_t *SaveLoadSettingsCallback(obs_data_t *save_data, bool saving);

StreamupHotkeyDisplaySettings::StreamupHotkeyDisplaySettings(QWidget *parent)
	: QDialog(parent),
	  layout(new QVBoxLayout(this)),
	  closeButton(new QPushButton("Close", this))
{
	setWindowTitle("Hotkey Display Settings");
	layout->addWidget(closeButton);
	setLayout(layout);

	connect(closeButton, &QPushButton::clicked, this, &StreamupHotkeyDisplaySettings::close);
}

void StreamupHotkeyDisplaySettings::LoadSettings(obs_data_t *settings)
{
	// Text GDI source selection
	textSource = QString::fromUtf8(obs_data_get_string(settings, "textSource"));

	// On screen time
	onScreenTime = obs_data_get_int(settings, "onScreenTime");
}

void StreamupHotkeyDisplaySettings::SaveSettings()
{
	obs_data_t *settings = obs_data_create();

	// Text GDI source selection
	obs_data_set_string(settings, "textSource", textSource.toUtf8().constData());

	// On screen time
	obs_data_set_int(settings, "onScreenTime", onScreenTime);

	SaveLoadSettingsCallback(settings, true);

	obs_data_release(settings);
}
