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
	  toggleButton(new QPushButton("Enable Hook", this)),
	  settingsButton(new QPushButton(this)),
	  hookEnabled(false),
	  clearTimer(new QTimer(this)),
	  sceneName("Default Scene"),
	  textSource("Default Text Source"),
	  onScreenTime(500),
	  prefix(""),
	  suffix(""),
	  displayInTextSource(false)
{
	label->setAlignment(Qt::AlignCenter);
	label->setStyleSheet("QLabel {"
			     "  border: 2px solid #888888;"
			     "  padding: 10px;"
			     "  border-radius: 10px;"
			     "  font-size: 18px;"
			     "  color: #FFFFFF;"
			     "  background-color: #333333;"
			     "}");
	label->setFixedHeight(50);
	layout->addWidget(label);

	toggleButton->setFixedHeight(30);
	settingsButton->setMinimumSize(26, 22);
	settingsButton->setMaximumSize(26, 22);
	settingsButton->setProperty("themeID", "configIconSmall");
	settingsButton->setIconSize(QSize(20, 20));

	buttonLayout->addWidget(toggleButton);
	buttonLayout->addWidget(settingsButton);
	layout->addLayout(buttonLayout);
	setLayout(layout);

	connect(toggleButton, &QPushButton::clicked, this, &HotkeyDisplayDock::toggleKeyboardHook);
	connect(settingsButton, &QPushButton::clicked, this, &HotkeyDisplayDock::openSettings);
	connect(clearTimer, &QTimer::timeout, this, &HotkeyDisplayDock::clearDisplay);

	// Load current settings
	obs_data_t *settings = SaveLoadSettingsCallback(nullptr, false);
	if (settings) {
		sceneName = QString::fromUtf8(obs_data_get_string(settings, "sceneName"));
		textSource = QString::fromUtf8(obs_data_get_string(settings, "textSource"));
		onScreenTime = obs_data_get_int(settings, "onScreenTime");
		prefix = QString::fromUtf8(obs_data_get_string(settings, "prefix"));
		suffix = QString::fromUtf8(obs_data_get_string(settings, "suffix"));
		displayInTextSource = obs_data_get_bool(settings, "displayInTextSource");
		hookEnabled = obs_data_get_bool(settings, "hookEnabled");

		if (hookEnabled) {
			keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, NULL, 0);
			if (!keyboardHook) {
				blog(LOG_ERROR, "[StreamUP Hotkey Display] Failed to set keyboard hook!");
				hookEnabled = false;
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

		obs_data_release(settings);
	} else {
		sceneName = "Default Scene";
		textSource = "Default Text Source";
		onScreenTime = 100;
		prefix = "";
		suffix = "";
		displayInTextSource = false;
	}
}

HotkeyDisplayDock::~HotkeyDisplayDock() {}

void HotkeyDisplayDock::setLog(const QString &log)
{
	// Always update the dock's label
	label->setText(log);

	// Conditionally update the text source based on the setting
	if (displayInTextSource) {
		if (sceneName == "Default Scene" || textSource == "Default Text Source" || textSource.isEmpty()) {
			blog(LOG_WARNING,
			     "[StreamUP Hotkey Display] Scene or text source is not selected or invalid. Skipping text update.");
			return;
		}

		if (textSource != "No text source available") {
			updateTextSource(log);
		}

		showSource();
	}

	// Restart the timer with the on-screen time value
	clearTimer->start(onScreenTime);
}

void HotkeyDisplayDock::toggleKeyboardHook()
{
	blog(LOG_INFO, "[StreamUP Hotkey Display] Toggling hook. Current state: %s", hookEnabled ? "Enabled" : "Disabled");

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
		stopAllActivities();
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

	// Save the hookEnabled state to settings
	obs_data_t *settings = SaveLoadSettingsCallback(nullptr, false);
	if (settings) {
		obs_data_set_bool(settings, "hookEnabled", hookEnabled);
		SaveLoadSettingsCallback(settings, true);
		obs_data_release(settings);
	}

	blog(LOG_INFO, "[StreamUP Hotkey Display] Hook toggled. New state: %s", hookEnabled ? "Enabled" : "Disabled");
}

void HotkeyDisplayDock::openSettings()
{
	StreamupHotkeyDisplaySettings *settingsDialog = new StreamupHotkeyDisplaySettings(this, this);

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
			prefix = QString::fromUtf8(obs_data_get_string(settings, "prefix"));
			suffix = QString::fromUtf8(obs_data_get_string(settings, "suffix"));
			obs_data_release(settings);
		}
	}

	delete settingsDialog;
}

void HotkeyDisplayDock::clearDisplay()
{
	label->clear();
	hideSource();
	resetToListeningState(); // Reset to listening state after clearing the display
}

void HotkeyDisplayDock::updateTextSource(const QString &text)
{
	if (!displayInTextSource || sceneName == "Default Scene" || textSource.isEmpty() ||
	    textSource == "No text source available") {
		blog(LOG_WARNING,
		     "[StreamUP Hotkey Display] Scene or text source is not selected or invalid. Skipping text update.");
		return;
	}

	if (sceneAndSourceExist() && !textSource.isEmpty()) {
		obs_source_t *source = obs_get_source_by_name(textSource.toUtf8().constData());
		if (source) {
			obs_data_t *settings = obs_source_get_settings(source);
			QString formattedText = prefix + text + suffix;
			obs_data_set_string(settings, "text", formattedText.toUtf8().constData());
			obs_source_update(source, settings);
			obs_data_release(settings);
			obs_source_release(source);
		} else {
			blog(LOG_WARNING, "[StreamUP Hotkey Display] Source '%s' not found!", textSource.toUtf8().constData());
		}
	} else {
		blog(LOG_WARNING, "[StreamUP Hotkey Display] Text source is empty or does not exist!");
	}
}

void HotkeyDisplayDock::showSource()
{
	if (!displayInTextSource) {
		return;
	}

	if (sceneName == "Default Scene" || textSource.isEmpty() || textSource == "No text source available") {
		blog(LOG_WARNING,
		     "[StreamUP Hotkey Display] Scene or text source is not selected or invalid. Skipping show source.");
		return;
	}

	if (sceneAndSourceExist()) {
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
		blog(LOG_WARNING, "[StreamUP Hotkey Display] Scene name or text source is empty or does not exist!");
	}
}

void HotkeyDisplayDock::hideSource()
{
	if (!displayInTextSource) {
		return;
	}

	if (sceneName == "Default Scene" || textSource.isEmpty() || textSource == "No text source available") {
		blog(LOG_WARNING,
		     "[StreamUP Hotkey Display] Scene or text source is not selected or invalid. Skipping hide source.");
		return;
	}

	if (sceneAndSourceExist()) {
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
		blog(LOG_WARNING, "[StreamUP Hotkey Display] Scene name or text source is empty or does not exist!");
	}
}

bool HotkeyDisplayDock::sceneAndSourceExist()
{
	if (sceneName.isEmpty() || textSource.isEmpty()) {
		if (displayInTextSource) {
			blog(LOG_WARNING, "[StreamUP Hotkey Display] Scene name or text source is empty!");
		}
		return false;
	}

	obs_source_t *scene = obs_get_source_by_name(sceneName.toUtf8().constData());
	if (!scene) {
		if (displayInTextSource) {
			blog(LOG_WARNING, "[StreamUP Hotkey Display] Scene '%s' does not exist!", sceneName.toUtf8().constData());
		}
		return false;
	}

	obs_scene_t *sceneAsScene = obs_scene_from_source(scene);
	obs_sceneitem_t *item = obs_scene_find_source(sceneAsScene, textSource.toUtf8().constData());
	obs_source_release(scene);

	if (!item) {
		if (displayInTextSource) {
			blog(LOG_WARNING, "[StreamUP Hotkey Display] Source '%s' does not exist in scene '%s'!",
			     textSource.toUtf8().constData(), sceneName.toUtf8().constData());
		}
		return false;
	}

	return true;
}

void HotkeyDisplayDock::stopAllActivities()
{
	if (clearTimer->isActive()) {
		clearTimer->stop();
	}
	label->clear();
}

void HotkeyDisplayDock::resetToListeningState()
{
	stopAllActivities();
	// Ensure the hook is enabled to listen for the next key press
	if (!keyboardHook) {
		keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, NULL, 0);
		if (!keyboardHook) {
			blog(LOG_ERROR, "[StreamUP Hotkey Display] Failed to set keyboard hook!");
		}
	}
}
