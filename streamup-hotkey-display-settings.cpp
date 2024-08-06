#include "streamup-hotkey-display-settings.hpp"
#include <obs-module.h>

extern obs_data_t *SaveLoadSettingsCallback(obs_data_t *save_data, bool saving);

StreamupHotkeyDisplaySettings::StreamupHotkeyDisplaySettings(HotkeyDisplayDock *dock, QWidget *parent)
	: QDialog(parent),
	  hotkeyDisplayDock(dock),
	  mainLayout(new QVBoxLayout(this)),
	  buttonLayout(new QHBoxLayout()),
	  sceneLayout(new QHBoxLayout()),
	  sourceLayout(new QHBoxLayout()),
	  timeLayout(new QHBoxLayout()),
	  prefixLayout(new QHBoxLayout()),
	  suffixLayout(new QHBoxLayout()),
	  sceneLabel(new QLabel(obs_module_text("SceneLabel"), this)),
	  sourceLabel(new QLabel(obs_module_text("TextSourceLabel"), this)),
	  timeLabel(new QLabel(obs_module_text("OnScreenTimeLabel"), this)),
	  prefixLabel(new QLabel(obs_module_text("PrefixLabel"), this)),
	  suffixLabel(new QLabel(obs_module_text("SuffixLabel"), this)),
	  sceneComboBox(new QComboBox(this)),
	  sourceComboBox(new QComboBox(this)),
	  timeSpinBox(new QSpinBox(this)),
	  prefixLineEdit(new QLineEdit(this)),
	  suffixLineEdit(new QLineEdit(this)),
	  applyButton(new QPushButton(obs_module_text("ApplyButton"), this)),
	  closeButton(new QPushButton(obs_module_text("CloseButton"), this)),
	  displayInTextSourceCheckBox(new QCheckBox(obs_module_text("DisplayHotkeysInTextSource"), this)),
	  textSourceGroupBox(new QGroupBox(obs_module_text("TextSourceSettings"), this))
{
	setWindowTitle(obs_module_text("HotkeyDisplaySettingsTitle"));

	setMinimumSize(300, 130);

	// Configure tooltips
	sceneComboBox->setToolTip(obs_module_text("SceneComboBoxTooltip"));
	sourceComboBox->setToolTip(obs_module_text("TextSourceComboBoxTooltip"));
	timeSpinBox->setToolTip(obs_module_text("OnScreenTimeTooltip"));
	prefixLineEdit->setToolTip(obs_module_text("PrefixTooltip"));
	suffixLineEdit->setToolTip(obs_module_text("SuffixTooltip"));
	applyButton->setToolTip(obs_module_text("ApplyButtonTooltip"));
	closeButton->setToolTip(obs_module_text("CloseButtonTooltip"));
	displayInTextSourceCheckBox->setToolTip(obs_module_text("DisplayHotkeysInTextSourceTooltip"));

	// Configure timeSpinBox
	timeSpinBox->setRange(100, 10000);
	timeSpinBox->setSingleStep(1);

	// Populate sceneComboBox
	PopulateSceneComboBox();

	// Add widgets to layouts
	sceneLayout->addWidget(sceneLabel);
	sceneLayout->addWidget(sceneComboBox);

	sourceLayout->addWidget(sourceLabel);
	sourceLayout->addWidget(sourceComboBox);

	prefixLayout->addWidget(prefixLabel);
	prefixLayout->addWidget(prefixLineEdit);

	suffixLayout->addWidget(suffixLabel);
	suffixLayout->addWidget(suffixLineEdit);

	// Create and configure textSourceGroupBox layout
	QVBoxLayout *textSourceLayout = new QVBoxLayout();
	textSourceLayout->addLayout(sceneLayout);
	textSourceLayout->addLayout(sourceLayout);
	textSourceLayout->addLayout(prefixLayout);
	textSourceLayout->addLayout(suffixLayout);
	textSourceGroupBox->setLayout(textSourceLayout);

	// Create and configure time layout
	QHBoxLayout *timeLayout = new QHBoxLayout();
	timeLayout->addWidget(timeLabel);
	timeLayout->addWidget(timeSpinBox);

	buttonLayout->addWidget(applyButton);
	buttonLayout->addWidget(closeButton);

	mainLayout->addWidget(displayInTextSourceCheckBox);
	mainLayout->addWidget(textSourceGroupBox); // Add the group box to the main layout
	mainLayout->addLayout(timeLayout);         // Add the time layout to the main layout
	mainLayout->addLayout(buttonLayout);
	setLayout(mainLayout);

	// Connect signals to slots
	connect(applyButton, &QPushButton::clicked, this, &StreamupHotkeyDisplaySettings::applySettings);
	connect(closeButton, &QPushButton::clicked, this, &StreamupHotkeyDisplaySettings::close);
	connect(sceneComboBox, &QComboBox::currentTextChanged, this, &StreamupHotkeyDisplaySettings::onSceneChanged);
	connect(displayInTextSourceCheckBox, &QCheckBox::toggled, this,
		&StreamupHotkeyDisplaySettings::onDisplayInTextSourceToggled); // Connect checkbox toggle

	// Load current settings
	obs_data_t *settings = SaveLoadSettingsCallback(nullptr, false);
	if (settings) {
		LoadSettings(settings);
		obs_data_release(settings);
	}
}

void StreamupHotkeyDisplaySettings::LoadSettings(obs_data_t *settings)
{
	// Existing settings
	sceneName = QString::fromUtf8(obs_data_get_string(settings, "sceneName"));
	sceneComboBox->setCurrentText(sceneName);
	PopulateSourceComboBox(sceneName);
	textSource = QString::fromUtf8(obs_data_get_string(settings, "textSource"));
	sourceComboBox->setCurrentText(textSource);
	onScreenTime = obs_data_get_int(settings, "onScreenTime");
	timeSpinBox->setValue(onScreenTime);
	displayInTextSource = obs_data_get_bool(settings, "displayInTextSource");
	displayInTextSourceCheckBox->setChecked(displayInTextSource);

	// New settings
	QString prefix = QString::fromUtf8(obs_data_get_string(settings, "prefix"));
	prefixLineEdit->setText(prefix);
	QString suffix = QString::fromUtf8(obs_data_get_string(settings, "suffix"));
	suffixLineEdit->setText(suffix);

	onDisplayInTextSourceToggled(displayInTextSource); // Set initial visibility of related settings
}

void StreamupHotkeyDisplaySettings::SaveSettings()
{
	obs_data_t *settings = obs_data_create();

	// Existing settings
	obs_data_set_string(settings, "sceneName", sceneComboBox->currentText().toUtf8().constData());
	obs_data_set_string(settings, "textSource", sourceComboBox->currentText().toUtf8().constData());
	obs_data_set_int(settings, "onScreenTime", timeSpinBox->value());
	obs_data_set_bool(settings, "displayInTextSource", displayInTextSourceCheckBox->isChecked());

	// New settings
	obs_data_set_string(settings, "prefix", prefixLineEdit->text().toUtf8().constData());
	obs_data_set_string(settings, "suffix", suffixLineEdit->text().toUtf8().constData());

	SaveLoadSettingsCallback(settings, true);
	obs_data_release(settings);
}

void StreamupHotkeyDisplaySettings::applySettings()
{
	sceneName = sceneComboBox->currentText();
	textSource = sourceComboBox->currentText();
	onScreenTime = timeSpinBox->value();
	displayInTextSource = displayInTextSourceCheckBox->isChecked();
	QString newPrefix = prefixLineEdit->text();
	QString newSuffix = suffixLineEdit->text();

	SaveSettings();

	if (hotkeyDisplayDock) {
		hotkeyDisplayDock->sceneName = sceneName;
		hotkeyDisplayDock->textSource = textSource;
		hotkeyDisplayDock->onScreenTime = onScreenTime;
		hotkeyDisplayDock->prefix = newPrefix;
		hotkeyDisplayDock->suffix = newSuffix;
		hotkeyDisplayDock->setDisplayInTextSource(displayInTextSource); // Apply the setting to the dock
	}

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

void StreamupHotkeyDisplaySettings::onDisplayInTextSourceToggled(bool checked)
{
	textSourceGroupBox->setVisible(checked);
	adjustSize();
}
