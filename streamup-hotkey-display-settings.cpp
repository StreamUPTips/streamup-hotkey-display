#include "streamup-hotkey-display-settings.hpp"
#include <obs-frontend-api.h>

extern obs_data_t *SaveLoadSettingsCallback(obs_data_t *save_data, bool saving);

StreamupHotkeyDisplaySettings::StreamupHotkeyDisplaySettings(QWidget *parent)
	: QDialog(parent),
	  mainLayout(new QVBoxLayout(this)),
	  buttonLayout(new QHBoxLayout()),
	  sourceLayout(new QHBoxLayout()),
	  timeLayout(new QHBoxLayout()),
	  sourceLabel(new QLabel("Text Source:", this)),
	  timeLabel(new QLabel("On Screen Time (ms):", this)),
	  sourceComboBox(new QComboBox(this)),
	  timeSpinBox(new QSpinBox(this)),
	  applyButton(new QPushButton("Apply", this)),
	  closeButton(new QPushButton("Close", this))
{
	setWindowTitle("Hotkey Display Settings");

	// Configure timeSpinBox
	timeSpinBox->setRange(0, 10000); // Range from 0ms to 10 seconds
	timeSpinBox->setSingleStep(1);   // Step of 1

	// Populate sourceComboBox (example, replace with actual source names)
	PopulateSourceComboBox();

	// Add widgets to layouts
	sourceLayout->addWidget(sourceLabel);
	sourceLayout->addWidget(sourceComboBox);

	timeLayout->addWidget(timeLabel);
	timeLayout->addWidget(timeSpinBox);

	buttonLayout->addWidget(applyButton);
	buttonLayout->addWidget(closeButton);

	mainLayout->addLayout(sourceLayout);
	mainLayout->addLayout(timeLayout);
	mainLayout->addLayout(buttonLayout);
	setLayout(mainLayout);

	// Connect buttons to their slots
	connect(applyButton, &QPushButton::clicked, this, &StreamupHotkeyDisplaySettings::applySettings);
	connect(closeButton, &QPushButton::clicked, this, &StreamupHotkeyDisplaySettings::close);
}

void StreamupHotkeyDisplaySettings::LoadSettings(obs_data_t *settings)
{
	// Text GDI source selection
	textSource = QString::fromUtf8(obs_data_get_string(settings, "textSource"));
	sourceComboBox->setCurrentText(textSource);

	// On screen time
	onScreenTime = obs_data_get_int(settings, "onScreenTime");
	timeSpinBox->setValue(onScreenTime);
}

void StreamupHotkeyDisplaySettings::SaveSettings()
{
	obs_data_t *settings = obs_data_create();

	// Text GDI source selection
	obs_data_set_string(settings, "textSource", sourceComboBox->currentText().toUtf8().constData());

	// On screen time
	obs_data_set_int(settings, "onScreenTime", timeSpinBox->value());

	SaveLoadSettingsCallback(settings, true);

	obs_data_release(settings);
}

void StreamupHotkeyDisplaySettings::applySettings()
{
	textSource = sourceComboBox->currentText();
	onScreenTime = timeSpinBox->value();
	SaveSettings();
	accept(); // Close the dialog
}

void StreamupHotkeyDisplaySettings::PopulateSourceComboBox()
{
	sourceComboBox->clear();

	obs_enum_sources(
		[](void *data, obs_source_t *source) {
			StreamupHotkeyDisplaySettings *settingsDialog = static_cast<StreamupHotkeyDisplaySettings *>(data);
			const char *sourceId = obs_source_get_id(source);

			if (strcmp(sourceId, "text_gdiplus") == 0 || strcmp(sourceId, "text_gdiplus_v3") == 0) {
				const char *sourceName = obs_source_get_name(source);
				settingsDialog->sourceComboBox->addItem(QString::fromUtf8(sourceName));
			}
			return true;
		},
		this);
}

