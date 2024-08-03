#include "streamup-hotkey-display-dock.hpp"
#include "streamup-hotkey-display-settings.hpp"
#include <windows.h>
#include <obs.h>
#include <QIcon>
#include <QThread>

extern obs_data_t *SaveLoadSettingsCallback(obs_data_t *save_data, bool saving);
extern HHOOK keyboardHook;
extern LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);

HotkeyDisplayDock::HotkeyDisplayDock(QWidget *parent)
	: QFrame(parent),
	  layout(new QVBoxLayout(this)),
	  buttonLayout(new QHBoxLayout()),
	  label(new QLabel(this)),
	  toggleButton(new QPushButton("Disable Hook", this)),
	  settingsButton(new QPushButton(this)),
	  hookEnabled(true),
	  clearTimer(new QTimer(this)),
	  onScreenTime(100) // Default value, adjust as needed
{
	label->setAlignment(Qt::AlignCenter);
	label->setStyleSheet("QLabel {"
			     "  border: 2px solid #4CAF50;"
			     "  padding: 10px;"
			     "  border-radius: 10px;"
			     "  font-size: 18px;"
			     "  color: #FFFFFF;"
			     "  background-color: #333333;"
			     "}");
	label->setFixedHeight(50);
	layout->addWidget(label);

	// Configure the toggle button
	toggleButton->setFixedHeight(30);

	// Configure the settings button with icon
	settingsButton->setMinimumSize(26, 22);
	settingsButton->setMaximumSize(26, 22);
	settingsButton->setProperty("themeID", "configIconSmall");
	settingsButton->setIconSize(QSize(20, 20));

	// Add the buttons to the horizontal layout
	buttonLayout->addWidget(toggleButton);
	buttonLayout->addWidget(settingsButton);

	// Add the horizontal layout to the main layout
	layout->addLayout(buttonLayout);

	setLayout(layout);

	// Connect the buttons to their slots
	connect(toggleButton, &QPushButton::clicked, this, &HotkeyDisplayDock::toggleKeyboardHook);
	connect(settingsButton, &QPushButton::clicked, this, &HotkeyDisplayDock::openSettings);

	// Connect the timer's timeout signal to the clearDisplay slot
	connect(clearTimer, &QTimer::timeout, this, &HotkeyDisplayDock::clearDisplay);

	// Load current settings
	obs_data_t *settings = SaveLoadSettingsCallback(nullptr, false);
	if (settings) {
		sceneName = QString::fromUtf8(obs_data_get_string(settings, "sceneName"));
		textSource = QString::fromUtf8(obs_data_get_string(settings, "textSource"));
		onScreenTime = obs_data_get_int(settings, "onScreenTime");
		obs_data_release(settings);
	}
}

HotkeyDisplayDock::~HotkeyDisplayDock() {}

void HotkeyDisplayDock::setLog(const QString &log)
{
	label->setText(log);
	updateTextSource(log);

	// Show the source when text is set
	showSource();

	// Delay for 50ms
	QThread::msleep(50);

	// Restart the timer with the on-screen time value
	clearTimer->start(onScreenTime);
}

void HotkeyDisplayDock::toggleKeyboardHook()
{
	if (hookEnabled) {
		if (keyboardHook) {
			UnhookWindowsHookEx(keyboardHook);
			keyboardHook = NULL;
		}
		toggleButton->setText("Enable Hook");
		label->setStyleSheet("QLabel {"
				     "  border: 2px solid #888888;"
				     "  padding: 10px;"
				     "  border-radius: 10px;"
				     "  font-size: 18px;"
				     "  color: #FFFFFF;"
				     "  background-color: #333333;"
				     "}");
	} else {
		keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, NULL, 0);
		if (!keyboardHook) {
			blog(LOG_ERROR, "[StreamUP Hotkey Display] Failed to set keyboard hook!");
		} else {
			toggleButton->setText("Disable Hook");
			label->setStyleSheet("QLabel {"
					     "  border: 2px solid #4CAF50;"
					     "  padding: 10px;"
					     "  border-radius: 10px;"
					     "  font-size: 18px;"
					     "  color: #FFFFFF;"
					     "  background-color: #333333;"
					     "}");
		}
	}
	hookEnabled = !hookEnabled;
}

void HotkeyDisplayDock::openSettings()
{
	StreamupHotkeyDisplaySettings *settingsDialog = new StreamupHotkeyDisplaySettings(this);

	// Load current settings
	obs_data_t *settings = SaveLoadSettingsCallback(nullptr, false);
	if (settings) {
		settingsDialog->LoadSettings(settings);
		obs_data_release(settings);
	}

	if (settingsDialog->exec() == QDialog::Accepted) {
		// Update the dock's settings after applying new settings
		obs_data_t *settings = SaveLoadSettingsCallback(nullptr, false);
		if (settings) {
			sceneName = QString::fromUtf8(obs_data_get_string(settings, "sceneName"));
			textSource = QString::fromUtf8(obs_data_get_string(settings, "textSource"));
			onScreenTime = obs_data_get_int(settings, "onScreenTime");
			obs_data_release(settings);
		}
	}

	delete settingsDialog;
}

void HotkeyDisplayDock::clearDisplay()
{
	label->clear();
	hideSource();
}

void HotkeyDisplayDock::updateTextSource(const QString &text)
{
	if (!textSource.isEmpty()) {
		obs_source_t *source = obs_get_source_by_name(textSource.toUtf8().constData());
		if (source) {
			obs_data_t *settings = obs_source_get_settings(source);
			obs_data_set_string(settings, "text", text.toUtf8().constData());
			obs_source_update(source, settings);
			obs_data_release(settings);
			obs_source_release(source);
		} else {
			blog(LOG_WARNING, "[StreamUP Hotkey Display] Source '%s' not found!", textSource.toUtf8().constData());
		}
	} else {
		blog(LOG_WARNING, "[StreamUP Hotkey Display] Text source is empty!");
	}
}

void HotkeyDisplayDock::showSource()
{
	if (!sceneName.isEmpty() && !textSource.isEmpty()) {
		obs_source_t *scene = obs_get_source_by_name(sceneName.toUtf8().constData());
		if (scene) {
			obs_scene_t *sceneAsScene = obs_scene_from_source(scene);
			obs_sceneitem_t *item = obs_scene_find_source(sceneAsScene, textSource.toUtf8().constData());
			if (item) {
				obs_sceneitem_set_visible(item, true);
			} else {
				blog(LOG_WARNING, "[StreamUP Hotkey Display] Scene item '%s' not found in scene '%s'!",
				     textSource.toUtf8().constData(), sceneName.toUtf8().constData());
			}
			obs_source_release(scene);
		} else {
			blog(LOG_WARNING, "[StreamUP Hotkey Display] Scene '%s' not found!", sceneName.toUtf8().constData());
		}
	} else {
		blog(LOG_WARNING, "[StreamUP Hotkey Display] Scene name or text source is empty!");
	}
}

void HotkeyDisplayDock::hideSource()
{
	if (!sceneName.isEmpty() && !textSource.isEmpty()) {
		obs_source_t *scene = obs_get_source_by_name(sceneName.toUtf8().constData());
		if (scene) {
			obs_scene_t *sceneAsScene = obs_scene_from_source(scene);
			obs_sceneitem_t *item = obs_scene_find_source(sceneAsScene, textSource.toUtf8().constData());
			if (item) {
				obs_sceneitem_set_visible(item, false);
			} else {
				blog(LOG_WARNING, "[StreamUP Hotkey Display] Scene item '%s' not found in scene '%s'!",
				     textSource.toUtf8().constData(), sceneName.toUtf8().constData());
			}
			obs_source_release(scene);
		} else {
			blog(LOG_WARNING, "[StreamUP Hotkey Display] Scene '%s' not found!", sceneName.toUtf8().constData());
		}
	} else {
		blog(LOG_WARNING, "[StreamUP Hotkey Display] Scene name or text source is empty!");
	}
}
