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
	  sceneLabel(new QLabel(obs_module_text("Settings.Label.Scene"), this)),
	  sourceLabel(new QLabel(obs_module_text("Settings.Label.TextSource"), this)),
	  timeLabel(new QLabel(obs_module_text("Settings.Label.OnScreenTime"), this)),
	  prefixLabel(new QLabel(obs_module_text("Settings.Label.Prefix"), this)),
	  suffixLabel(new QLabel(obs_module_text("Settings.Label.Suffix"), this)),
	  prefixLineEdit(new QLineEdit(this)),
	  suffixLineEdit(new QLineEdit(this)),
	  sceneComboBox(new QComboBox(this)),
	  sourceComboBox(new QComboBox(this)),
	  timeSpinBox(new QSpinBox(this)),
	  applyButton(new QPushButton(obs_module_text("Settings.Button.Apply"), this)),
	  closeButton(new QPushButton(obs_module_text("Settings.Button.Close"), this)),
	  displayInTextSourceCheckBox(new QCheckBox(obs_module_text("Settings.Checkbox.DisplayInTextSource"), this)),
	  textSourceGroupBox(new QGroupBox(obs_module_text("Settings.Group.TextSource"), this)),
	  singleKeyGroupBox(new QGroupBox(obs_module_text("Settings.Group.SingleKeyCapture"), this)),
	  captureNumpadCheckBox(new QCheckBox(obs_module_text("Settings.Checkbox.CaptureNumpad"), this)),
	  captureNumbersCheckBox(new QCheckBox(obs_module_text("Settings.Checkbox.CaptureNumbers"), this)),
	  captureLettersCheckBox(new QCheckBox(obs_module_text("Settings.Checkbox.CaptureLetters"), this)),
	  capturePunctuationCheckBox(new QCheckBox(obs_module_text("Settings.Checkbox.CapturePunctuation"), this)),
	  whitelistLabel(new QLabel(obs_module_text("Settings.Label.Whitelist"), this)),
	  whitelistLineEdit(new QLineEdit(this)),
	  enableLoggingCheckBox(new QCheckBox(obs_module_text("Settings.Checkbox.EnableLogging"), this))
{
	setWindowTitle(obs_module_text("Settings.Title"));
	setAccessibleName(obs_module_text("Settings.Title"));
	setAccessibleDescription(obs_module_text("Settings.Description"));

	setMinimumSize(300, 130);

	// Configure tooltips and accessibility
	sceneComboBox->setToolTip(obs_module_text("Settings.Tooltip.Scene"));
	sceneComboBox->setAccessibleName(obs_module_text("Settings.Label.Scene"));
	sceneComboBox->setAccessibleDescription(obs_module_text("Settings.Tooltip.Scene"));

	sourceComboBox->setToolTip(obs_module_text("Settings.Tooltip.TextSource"));
	sourceComboBox->setAccessibleName(obs_module_text("Settings.Label.TextSource"));
	sourceComboBox->setAccessibleDescription(obs_module_text("Settings.Tooltip.TextSource"));

	timeSpinBox->setToolTip(obs_module_text("Settings.Tooltip.OnScreenTime"));
	timeSpinBox->setAccessibleName(obs_module_text("Settings.Label.OnScreenTime"));
	timeSpinBox->setAccessibleDescription(obs_module_text("Settings.Tooltip.OnScreenTime"));

	prefixLineEdit->setToolTip(obs_module_text("Settings.Tooltip.Prefix"));
	prefixLineEdit->setAccessibleName(obs_module_text("Settings.Label.Prefix"));
	prefixLineEdit->setAccessibleDescription(obs_module_text("Settings.Tooltip.Prefix"));
	prefixLineEdit->setPlaceholderText(obs_module_text("Settings.Placeholder.Prefix"));

	suffixLineEdit->setToolTip(obs_module_text("Settings.Tooltip.Suffix"));
	suffixLineEdit->setAccessibleName(obs_module_text("Settings.Label.Suffix"));
	suffixLineEdit->setAccessibleDescription(obs_module_text("Settings.Tooltip.Suffix"));
	suffixLineEdit->setPlaceholderText(obs_module_text("Settings.Placeholder.Suffix"));

	applyButton->setToolTip(obs_module_text("Settings.Tooltip.Apply"));
	applyButton->setAccessibleName(obs_module_text("Settings.Button.Apply"));
	applyButton->setAccessibleDescription(obs_module_text("Settings.Tooltip.Apply"));
	applyButton->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Return));

	closeButton->setToolTip(obs_module_text("Settings.Tooltip.Close"));
	closeButton->setAccessibleName(obs_module_text("Settings.Button.Close"));
	closeButton->setAccessibleDescription(obs_module_text("Settings.Tooltip.Close"));
	closeButton->setShortcut(QKeySequence(Qt::Key_Escape));

	displayInTextSourceCheckBox->setToolTip(obs_module_text("Settings.Tooltip.DisplayInTextSource"));
	displayInTextSourceCheckBox->setAccessibleName(obs_module_text("Settings.Checkbox.DisplayInTextSource"));
	displayInTextSourceCheckBox->setAccessibleDescription(obs_module_text("Settings.Tooltip.DisplayInTextSource"));

	textSourceGroupBox->setAccessibleName(obs_module_text("Settings.Group.TextSource"));

	// Set accessible properties for labels
	sceneLabel->setAccessibleName(obs_module_text("Settings.Label.Scene"));
	sourceLabel->setAccessibleName(obs_module_text("Settings.Label.TextSource"));
	timeLabel->setAccessibleName(obs_module_text("Settings.Label.OnScreenTime"));
	prefixLabel->setAccessibleName(obs_module_text("Settings.Label.Prefix"));
	suffixLabel->setAccessibleName(obs_module_text("Settings.Label.Suffix"));

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

	// Create and configure singleKeyGroupBox layout
	QVBoxLayout *singleKeyLayout = new QVBoxLayout();
	singleKeyLayout->addWidget(captureNumpadCheckBox);
	singleKeyLayout->addWidget(captureNumbersCheckBox);
	singleKeyLayout->addWidget(captureLettersCheckBox);
	singleKeyLayout->addWidget(capturePunctuationCheckBox);
	singleKeyLayout->addWidget(whitelistLabel);
	singleKeyLayout->addWidget(whitelistLineEdit);
	singleKeyGroupBox->setLayout(singleKeyLayout);

	// Set tooltips for single key capture options
	captureNumpadCheckBox->setToolTip(obs_module_text("Settings.Tooltip.CaptureNumpad"));
	captureNumbersCheckBox->setToolTip(obs_module_text("Settings.Tooltip.CaptureNumbers"));
	captureLettersCheckBox->setToolTip(obs_module_text("Settings.Tooltip.CaptureLetters"));
	capturePunctuationCheckBox->setToolTip(obs_module_text("Settings.Tooltip.CapturePunctuation"));
	whitelistLineEdit->setToolTip(obs_module_text("Settings.Tooltip.Whitelist"));
	whitelistLineEdit->setPlaceholderText(obs_module_text("Settings.Placeholder.Whitelist"));

	// Set tooltip for logging checkbox
	enableLoggingCheckBox->setToolTip(obs_module_text("Settings.Tooltip.EnableLogging"));

	mainLayout->addWidget(displayInTextSourceCheckBox);
	mainLayout->addWidget(textSourceGroupBox); // Add the group box to the main layout
	mainLayout->addWidget(singleKeyGroupBox); // Add the single key capture group box
	mainLayout->addWidget(enableLoggingCheckBox); // Add the logging checkbox
	mainLayout->addLayout(timeLayout);         // Add the time layout to the main layout
	mainLayout->addLayout(buttonLayout);
	setLayout(mainLayout);

	// Set up proper tab order for keyboard navigation
	setTabOrder(displayInTextSourceCheckBox, sceneComboBox);
	setTabOrder(sceneComboBox, sourceComboBox);
	setTabOrder(sourceComboBox, prefixLineEdit);
	setTabOrder(prefixLineEdit, suffixLineEdit);
	setTabOrder(suffixLineEdit, timeSpinBox);
	setTabOrder(timeSpinBox, applyButton);
	setTabOrder(applyButton, closeButton);

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

	// Single key capture settings
	captureNumpad = obs_data_get_bool(settings, "captureNumpad");
	captureNumpadCheckBox->setChecked(captureNumpad);
	captureNumbers = obs_data_get_bool(settings, "captureNumbers");
	captureNumbersCheckBox->setChecked(captureNumbers);
	captureLetters = obs_data_get_bool(settings, "captureLetters");
	captureLettersCheckBox->setChecked(captureLetters);
	capturePunctuation = obs_data_get_bool(settings, "capturePunctuation");
	capturePunctuationCheckBox->setChecked(capturePunctuation);
	whitelistedKeys = QString::fromUtf8(obs_data_get_string(settings, "whitelistedKeys"));
	whitelistLineEdit->setText(whitelistedKeys);

	// Logging settings (default to false if not present)
	enableLogging = obs_data_get_bool(settings, "enableLogging");
	enableLoggingCheckBox->setChecked(enableLogging);

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

	// Single key capture settings
	obs_data_set_bool(settings, "captureNumpad", captureNumpadCheckBox->isChecked());
	obs_data_set_bool(settings, "captureNumbers", captureNumbersCheckBox->isChecked());
	obs_data_set_bool(settings, "captureLetters", captureLettersCheckBox->isChecked());
	obs_data_set_bool(settings, "capturePunctuation", capturePunctuationCheckBox->isChecked());
	obs_data_set_string(settings, "whitelistedKeys", whitelistLineEdit->text().toUtf8().constData());

	// Logging settings
	obs_data_set_bool(settings, "enableLogging", enableLoggingCheckBox->isChecked());

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

	// Single key capture settings
	captureNumpad = captureNumpadCheckBox->isChecked();
	captureNumbers = captureNumbersCheckBox->isChecked();
	captureLetters = captureLettersCheckBox->isChecked();
	capturePunctuation = capturePunctuationCheckBox->isChecked();
	whitelistedKeys = whitelistLineEdit->text();

	// Logging settings
	enableLogging = enableLoggingCheckBox->isChecked();

	SaveSettings();

	if (hotkeyDisplayDock) {
		hotkeyDisplayDock->sceneName = sceneName;
		hotkeyDisplayDock->textSource = textSource;
		hotkeyDisplayDock->onScreenTime = onScreenTime;
		hotkeyDisplayDock->prefix = newPrefix;
		hotkeyDisplayDock->suffix = newSuffix;
		hotkeyDisplayDock->setDisplayInTextSource(displayInTextSource); // Apply the setting to the dock
	}

	// Reload settings to update global single key capture variables
	obs_data_t *reloadedSettings = SaveLoadSettingsCallback(nullptr, false);
	if (reloadedSettings) {
		extern void loadSingleKeyCaptureSettings(obs_data_t *settings);
		loadSingleKeyCaptureSettings(reloadedSettings);
		obs_data_release(reloadedSettings);
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

	struct obs_frontend_source_list scenes = {{{0}}};
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
		sourceComboBox->addItem(StyleConstants::NO_TEXT_SOURCE);
		return;
	}

	obs_scene_t *sceneAsScene = obs_scene_from_source(scene);
	obs_scene_enum_items(
		sceneAsScene,
		[](obs_scene_t * /*scene*/, obs_sceneitem_t *item, void *param) {
			StreamupHotkeyDisplaySettings *settingsDialog = static_cast<StreamupHotkeyDisplaySettings *>(param);
			obs_source_t *source = obs_sceneitem_get_source(item);
			const char *sourceId = obs_source_get_id(source);

			// Support all text source types across platforms
			// Windows: text_gdiplus, text_gdiplus_v3
			// Cross-platform: text_ft2_source_v2, text_ft2_source
			// Linux: text_pango_source
			if (strcmp(sourceId, "text_gdiplus") == 0 ||
			    strcmp(sourceId, "text_gdiplus_v3") == 0 ||
			    strcmp(sourceId, "text_ft2_source_v2") == 0 ||
			    strcmp(sourceId, "text_ft2_source") == 0 ||
			    strcmp(sourceId, "text_pango_source") == 0) {
				const char *sourceName = obs_source_get_name(source);
				settingsDialog->sourceComboBox->addItem(QString::fromUtf8(sourceName));
			}
			return true;
		},
		this);

	obs_source_release(scene);

	if (sourceComboBox->count() == 0) {
		sourceComboBox->addItem(StyleConstants::NO_TEXT_SOURCE);
	}
}

void StreamupHotkeyDisplaySettings::onDisplayInTextSourceToggled(bool checked)
{
	textSourceGroupBox->setVisible(checked);
	adjustSize();
}
