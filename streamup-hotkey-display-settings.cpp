#include "streamup-hotkey-display-settings.hpp"
#include <obs-frontend-api.h>

extern obs_data_t *SaveLoadSettingsCallback(obs_data_t *save_data, bool saving);

StreamupHotkeyDisplaySettings::StreamupHotkeyDisplaySettings(QWidget *parent)
	: QDialog(parent),
	  mainLayout(new QVBoxLayout(this)),
	  buttonLayout(new QHBoxLayout()),
	  sceneLayout(new QHBoxLayout()),
	  sourceLayout(new QHBoxLayout()),
	  timeLayout(new QHBoxLayout()),
	  sceneLabel(new QLabel("Scene:", this)),
	  sourceLabel(new QLabel("Text Source:", this)),
	  timeLabel(new QLabel("On Screen Time (ms):", this)),
	  sceneComboBox(new QComboBox(this)),
	  sourceComboBox(new QComboBox(this)),
	  timeSpinBox(new QSpinBox(this)),
	  applyButton(new QPushButton("Apply", this)),
	  closeButton(new QPushButton("Close", this))
{
	setWindowTitle("Hotkey Display Settings");

	setMinimumSize(300, 220);

	// Configure timeSpinBox
	timeSpinBox->setRange(0, 10000); // Range from 0ms to 10 seconds
	timeSpinBox->setSingleStep(1);   // Step of 1

	// Populate sceneComboBox
	PopulateSceneComboBox();

	// Add widgets to layouts
	sceneLayout->addWidget(sceneLabel);
	sceneLayout->addWidget(sceneComboBox);

	sourceLayout->addWidget(sourceLabel);
	sourceLayout->addWidget(sourceComboBox);

	timeLayout->addWidget(timeLabel);
	timeLayout->addWidget(timeSpinBox);

	buttonLayout->addWidget(applyButton);
	buttonLayout->addWidget(closeButton);

	mainLayout->addLayout(sceneLayout);
	mainLayout->addLayout(sourceLayout);
	mainLayout->addLayout(timeLayout);
	mainLayout->addLayout(buttonLayout);
	setLayout(mainLayout);

	// Connect signals to slots
	connect(applyButton, &QPushButton::clicked, this, &StreamupHotkeyDisplaySettings::applySettings);
	connect(closeButton, &QPushButton::clicked, this, &StreamupHotkeyDisplaySettings::close);
	connect(sceneComboBox, &QComboBox::currentTextChanged, this, &StreamupHotkeyDisplaySettings::onSceneChanged);
}

void StreamupHotkeyDisplaySettings::LoadSettings(obs_data_t *settings)
{
	// Scene selection
	sceneName = QString::fromUtf8(obs_data_get_string(settings, "sceneName"));
	sceneComboBox->setCurrentText(sceneName);

	// Populate source combo box based on the selected scene
	PopulateSourceComboBox(sceneName);

	// Text source selection
	textSource = QString::fromUtf8(obs_data_get_string(settings, "textSource"));
	sourceComboBox->setCurrentText(textSource);

	// On screen time
	onScreenTime = obs_data_get_int(settings, "onScreenTime");
	timeSpinBox->setValue(onScreenTime);
}

void StreamupHotkeyDisplaySettings::SaveSettings()
{
	obs_data_t *settings = obs_data_create();

	// Scene selection
	obs_data_set_string(settings, "sceneName", sceneComboBox->currentText().toUtf8().constData());

	// Text source selection
	obs_data_set_string(settings, "textSource", sourceComboBox->currentText().toUtf8().constData());

	// On screen time
	obs_data_set_int(settings, "onScreenTime", timeSpinBox->value());

	SaveLoadSettingsCallback(settings, true);

	obs_data_release(settings);
}

void StreamupHotkeyDisplaySettings::applySettings()
{
	sceneName = sceneComboBox->currentText();
	textSource = sourceComboBox->currentText();
	onScreenTime = timeSpinBox->value();
	SaveSettings();
	accept(); // Close the dialog
}

void StreamupHotkeyDisplaySettings::onSceneChanged(const QString &sceneName)
{
	QString previousSource = sourceComboBox->currentText();
	PopulateSourceComboBox(sceneName);
	if (!previousSource.isEmpty()) {
		sourceComboBox->setCurrentText(previousSource);
	}
}

void StreamupHotkeyDisplaySettings::PopulateSceneComboBox()
{
	sceneComboBox->clear();

	struct obs_frontend_source_list scenes = {0};
	obs_frontend_get_scenes(&scenes);

	for (size_t i = 0; i < scenes.sources.num; i++) {
		obs_source_t *source = scenes.sources.array[i];
		const char *name = obs_source_get_name(source);
		sceneComboBox->addItem(QString::fromUtf8(name));
	}

	obs_frontend_source_list_free(&scenes);
}

void StreamupHotkeyDisplaySettings::PopulateSourceComboBox(const QString &sceneName)
{
	sourceComboBox->clear();

	obs_source_t *scene = obs_get_source_by_name(sceneName.toUtf8().constData());
	if (!scene) {
		sourceComboBox->addItem("No text source available");
		return;
	}

	obs_scene_t *sceneAsScene = obs_scene_from_source(scene);
	obs_scene_enum_items(
		sceneAsScene,
		[](obs_scene_t *scene, obs_sceneitem_t *item, void *param) {
			StreamupHotkeyDisplaySettings *settingsDialog = static_cast<StreamupHotkeyDisplaySettings *>(param);
			obs_source_t *source = obs_sceneitem_get_source(item);
			const char *sourceId = obs_source_get_id(source);
			if (strcmp(sourceId, "text_gdiplus") == 0 || strcmp(sourceId, "text_gdiplus_v3") == 0) {
				const char *sourceName = obs_source_get_name(source);
				settingsDialog->sourceComboBox->addItem(QString::fromUtf8(sourceName));
			}
			return true;
		},
		this);

	obs_source_release(scene);

	if (sourceComboBox->count() == 0) {
		sourceComboBox->addItem("No text source available");
	}
}
