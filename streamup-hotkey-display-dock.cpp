#include "streamup-hotkey-display-dock.hpp"
#include "streamup-hotkey-display-settings.hpp"
#include <obs.h>
#include <QIcon>
#include <QStyle>
#include <QToolButton>
#include <QThread>
#include <obs-module.h>

#ifdef _WIN32
#include <windows.h>
extern HHOOK keyboardHook;
extern HHOOK mouseHook;
extern LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
extern LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam);
#endif

#ifdef __APPLE__
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
extern CFMachPortRef eventTap;
extern void startMacOSKeyboardHook();
extern void stopMacOSKeyboardHook();
CGEventRef CGEventCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *refcon);
#endif

#ifdef __linux__
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <atomic>
extern Display *display;
extern std::atomic<bool> linuxHookRunning;
extern void startLinuxKeyboardHook();
extern void stopLinuxKeyboardHook();
#endif

extern obs_data_t *SaveLoadSettingsCallback(obs_data_t *save_data, bool saving);

HotkeyDisplayDock::HotkeyDisplayDock(QWidget *parent)
	: QFrame(parent),
	  layout(new QVBoxLayout(this)),
	  toolbar(new QToolBar(this)),
	  label(new QLabel(this)),
	  toggleAction(new QAction(this)),
	  settingsAction(new QAction(this)),
	  hookEnabled(false),
	  sceneName(StyleConstants::DEFAULT_SCENE_NAME),
	  textSource(StyleConstants::DEFAULT_TEXT_SOURCE),
	  onScreenTime(StyleConstants::DEFAULT_ONSCREEN_TIME),
	  prefix(""),
	  suffix(""),
	  clearTimer(new QTimer(this)),
	  displayInTextSource(false)
{
	// Set object names for theme styling
	setObjectName("hotkeyDisplayDock");
	layout->setObjectName("hotkeyDisplayLayout");
	toolbar->setObjectName("hotkeyDisplayToolbar");
	label->setObjectName("hotkeyDisplayLabel");

	// Set frame properties (matching OBS dock structure)
	setContentsMargins(0, 0, 0, 0);
	setMinimumWidth(100);
	setMinimumHeight(50);

	// Set layout properties - no margins/spacing (we'll add them manually)
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);

	// Add top spacing (8px above label)
	layout->addSpacing(8);

	// Create a horizontal layout to add left/right margins to the label only
	QHBoxLayout *labelLayout = new QHBoxLayout();
	labelLayout->setObjectName("hotkeyDisplayLabelLayout");
	labelLayout->setContentsMargins(8, 0, 8, 0); // 8px left and right margins
	labelLayout->setSpacing(0);

	label->setAlignment(Qt::AlignCenter);
	label->setFrameShape(QFrame::NoFrame);
	label->setFrameShadow(QFrame::Plain);
	label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
	label->setMinimumHeight(50);
	label->setWordWrap(true);
	label->setProperty("hotkeyState", "inactive");

	// Use theme-aware styling
	label->setStyleSheet(
		"QLabel[hotkeyState=\"active\"] {"
		"  border: 2px solid palette(highlight);"
		"  border-radius: 4px;"
		"  padding: 10px;"
		"  font-size: 14pt;"
		"  background: palette(base);"
		"}"
		"QLabel[hotkeyState=\"inactive\"] {"
		"  border: 2px solid palette(mid);"
		"  border-radius: 4px;"
		"  padding: 10px;"
		"  font-size: 14pt;"
		"  background: palette(base);"
		"}"
	);

	// Add label to the horizontal layout
	labelLayout->addWidget(label);

	// Add the label layout to main layout with stretch factor 1 (expands to fill space)
	layout->addLayout(labelLayout, 1);

	// Add spacing between label and toolbar (8px)
	layout->addSpacing(8);

	// Configure toolbar (matching OBS style)
	toolbar->setMovable(false);
	toolbar->setFloatable(false);
	toolbar->setIconSize(QSize(16, 16));

	// Configure toggle action
	toggleAction->setText(obs_module_text("Dock.Button.Enable"));
	toggleAction->setCheckable(true);
	toggleAction->setChecked(false);
	toggleAction->setToolTip(obs_module_text("Dock.Tooltip.Enable"));
	toggleAction->setProperty("class", "icon-toggle");

	// Configure settings action
	toggleAction->setObjectName("hotkeyDisplayToggleAction");
	settingsAction->setObjectName("hotkeyDisplaySettingsAction");
	settingsAction->setToolTip(obs_module_text("Dock.Tooltip.Settings"));
	settingsAction->setProperty("themeID", "configIconSmall");
	settingsAction->setProperty("class", "icon-gear");

	// Set accessible properties for the display label
	label->setAccessibleName(obs_module_text("Dock.Description"));
	label->setAccessibleDescription(obs_module_text("Dock.Label.Idle"));

	// Add actions to toolbar
	toolbar->addAction(toggleAction);
	toolbar->addSeparator();
	toolbar->addAction(settingsAction);

	// Copy dynamic properties from actions to widgets (OBS pattern)
	for (QAction *action : toolbar->actions()) {
		QWidget *widget = toolbar->widgetForAction(action);
		if (!widget)
			continue;

		for (QByteArray &propName : action->dynamicPropertyNames()) {
			widget->setProperty(propName, action->property(propName));
		}

		// Force style refresh to apply properties
		if (QStyle *style = widget->style()) {
			style->unpolish(widget);
			style->polish(widget);
		}
	}

	// Configure toggle button to show text (not just icon)
	QWidget *toggleWidget = toolbar->widgetForAction(toggleAction);
	if (toggleWidget) {
		QToolButton *toggleToolButton = qobject_cast<QToolButton *>(toggleWidget);
		if (toggleToolButton) {
			toggleToolButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
			toggleToolButton->setObjectName("hotkeyDisplayToggleButton");
			toggleToolButton->setMinimumWidth(100); // Ensure enough space for text
			toggleToolButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
		}
	}

	// Set object name for settings button widget
	QWidget *settingsWidget = toolbar->widgetForAction(settingsAction);
	if (settingsWidget) {
		settingsWidget->setObjectName("hotkeyDisplaySettingsButton");
	}

	// Add toolbar with stretch factor 0 (fixed height at bottom)
	layout->addWidget(toolbar, 0);

	setLayout(layout);

	connect(toggleAction, &QAction::triggered, this, &HotkeyDisplayDock::toggleKeyboardHook);
	connect(settingsAction, &QAction::triggered, this, &HotkeyDisplayDock::openSettings);
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

		// Use helper function to enable hooks if configured
		if (hookEnabled) {
			if (enableHooks()) {
				updateUIState(true);
			} else {
				hookEnabled = false;
				updateUIState(false);
			}
		}

		obs_data_release(settings);
	} else {
		sceneName = StyleConstants::DEFAULT_SCENE_NAME;
		textSource = StyleConstants::DEFAULT_TEXT_SOURCE;
		onScreenTime = StyleConstants::DEFAULT_ONSCREEN_TIME;
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
		if (sceneName == StyleConstants::DEFAULT_SCENE_NAME || textSource == StyleConstants::DEFAULT_TEXT_SOURCE || textSource.isEmpty()) {
			blog(LOG_WARNING,
			     "[StreamUP Hotkey Display] Scene or text source is not selected or invalid. Skipping text update.");
			return;
		}

		if (textSource != StyleConstants::NO_TEXT_SOURCE) {
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

	// Get the desired state from the toggle action
	bool shouldEnable = toggleAction->isChecked();

	if (shouldEnable) {
		// Enable hooks
		if (enableHooks()) {
			hookEnabled = true;
			updateUIState(true);
		} else {
			// Failed to enable, revert action state
			hookEnabled = false;
			toggleAction->setChecked(false);
			updateUIState(false);
		}
	} else {
		// Disable hooks
		disableHooks();
		hookEnabled = false;
		updateUIState(false);
	}

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
	if (!displayInTextSource || sceneName == StyleConstants::DEFAULT_SCENE_NAME || textSource.isEmpty() ||
	    textSource == StyleConstants::NO_TEXT_SOURCE) {
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

	if (sceneName == StyleConstants::DEFAULT_SCENE_NAME || textSource.isEmpty() || textSource == StyleConstants::NO_TEXT_SOURCE) {
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

	if (sceneName == StyleConstants::DEFAULT_SCENE_NAME || textSource.isEmpty() || textSource == StyleConstants::NO_TEXT_SOURCE) {
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
#ifdef _WIN32
	if (!keyboardHook) {
		keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, NULL, 0);
		if (!keyboardHook) {
			blog(LOG_ERROR, "[StreamUP Hotkey Display] Failed to set keyboard hook!");
		}
	}
#endif

#ifdef __APPLE__
	if (!eventTap) {
		startMacOSKeyboardHook();
		if (!eventTap) {
			blog(LOG_ERROR, "[StreamUP Hotkey Display] Failed to create event tap!");
		}
	}
#endif

#ifdef __linux__
	if (!display) {
		startLinuxKeyboardHook();
		if (!display) {
			blog(LOG_ERROR, "[StreamUP Hotkey Display] Failed to open X display!");
		}
	}
#endif
}

bool HotkeyDisplayDock::enableHooks()
{
#ifdef _WIN32
	mouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseProc, NULL, 0);
	if (!mouseHook) {
		blog(LOG_ERROR, "[StreamUP Hotkey Display] Failed to set mouse hook!");
	}
	keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, NULL, 0);
	if (!keyboardHook) {
		blog(LOG_ERROR, "[StreamUP Hotkey Display] Failed to set keyboard hook!");
		if (mouseHook) {
			UnhookWindowsHookEx(mouseHook);
			mouseHook = NULL;
		}
		return false;
	}
	return true;
#endif

#ifdef __APPLE__
	startMacOSKeyboardHook();
	if (!eventTap) {
		blog(LOG_ERROR, "[StreamUP Hotkey Display] Failed to create event tap!");
		return false;
	}
	return true;
#endif

#ifdef __linux__
	startLinuxKeyboardHook();
	if (!display && !linuxHookRunning) {
		blog(LOG_ERROR, "[StreamUP Hotkey Display] Failed to open X display!");
		return false;
	}
	return true;
#endif

	return false;
}

void HotkeyDisplayDock::disableHooks()
{
#ifdef _WIN32
	if (mouseHook) {
		UnhookWindowsHookEx(mouseHook);
		mouseHook = NULL;
	}
	if (keyboardHook) {
		UnhookWindowsHookEx(keyboardHook);
		keyboardHook = NULL;
	}
#endif

#ifdef __APPLE__
	stopMacOSKeyboardHook();
#endif

#ifdef __linux__
	stopLinuxKeyboardHook();
#endif

	stopAllActivities();
}

void HotkeyDisplayDock::updateUIState(bool enabled)
{
	const char *actionText = enabled ? obs_module_text("Dock.Button.Disable") : obs_module_text("Dock.Button.Enable");
	const char *actionTooltip = enabled ? obs_module_text("Dock.Tooltip.Disable") : obs_module_text("Dock.Tooltip.Enable");
	const char *labelDesc = enabled ? obs_module_text("Dock.Label.Active") : obs_module_text("Dock.Label.Idle");
	const char *state = enabled ? "active" : "inactive";

	toggleAction->setChecked(enabled);
	toggleAction->setText(actionText);
	toggleAction->setToolTip(actionTooltip);

	label->setProperty("hotkeyState", state);
	label->setAccessibleDescription(labelDesc);
	label->style()->unpolish(label);
	label->style()->polish(label);
}
