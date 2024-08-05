#include "streamup-hotkey-display-dock.hpp"
#include "streamup-hotkey-display-settings.hpp"
#include "version.h"
#include <obs-module.h>
#include <obs-frontend-api.h>
#include <obs.h>
#include <windows.h>
#include <unordered_set>
#include <string>
#include <vector>
#include <QMainWindow>
#include <QDockWidget>
#include <util/platform.h>
#include "obs-websocket-api.h"

#define QT_UTF8(str) QString::fromUtf8(str)
#define QT_TO_UTF8(str) str.toUtf8().constData()

OBS_DECLARE_MODULE()
OBS_MODULE_AUTHOR("Andilippi")
OBS_MODULE_USE_DEFAULT_LOCALE("streamup-hotkey-display", "en-US")

HHOOK keyboardHook; // Define the keyboard hook
std::unordered_set<int> pressedKeys;
std::unordered_set<int> activeModifiers;
std::unordered_set<int> modifierKeys = {VK_CONTROL, VK_LCONTROL, VK_RCONTROL, VK_MENU, VK_LMENU, VK_RMENU,
					VK_SHIFT,   VK_LSHIFT,   VK_RSHIFT,   VK_LWIN, VK_RWIN};

std::unordered_set<int> singleKeys = {VK_INSERT, VK_DELETE, VK_HOME, VK_END, VK_PRIOR, VK_NEXT, VK_F1,  VK_F2,  VK_F3,
				      VK_F4,     VK_F5,     VK_F6,   VK_F7,  VK_F8,    VK_F9,   VK_F10, VK_F11, VK_F12};

std::unordered_set<std::string> loggedCombinations;

HotkeyDisplayDock *hotkeyDisplayDock = nullptr;
StreamupHotkeyDisplaySettings *settingsDialog = nullptr; // Ensure this is defined
obs_websocket_vendor websocket_vendor = nullptr;         // Declare a vendor handle

bool isModifierKeyPressed()
{
	for (int key : activeModifiers) {
		if (pressedKeys.count(key)) {
			return true;
		}
	}
	return false;
}

std::string getKeyName(int vkCode)
{
	switch (vkCode) {
	case VK_CONTROL:
	case VK_LCONTROL:
	case VK_RCONTROL:
		return "Ctrl";
	case VK_MENU:
	case VK_LMENU:
	case VK_RMENU:
		return "Alt";
	case VK_SHIFT:
	case VK_LSHIFT:
	case VK_RSHIFT:
		return "Shift";
	case VK_LWIN:
	case VK_RWIN:
		return "Win";
	case VK_RETURN:
		return "Enter";
	case VK_SPACE:
		return "Space";
	case VK_BACK:
		return "Backspace";
	case VK_TAB:
		return "Tab";
	case VK_ESCAPE:
		return "Escape";
	case VK_PRIOR:
		return "Page Up";
	case VK_NEXT:
		return "Page Down";
	case VK_END:
		return "End";
	case VK_HOME:
		return "Home";
	case VK_LEFT:
		return "Left Arrow";
	case VK_UP:
		return "Up Arrow";
	case VK_RIGHT:
		return "Right Arrow";
	case VK_DOWN:
		return "Down Arrow";
	case VK_INSERT:
		return "Insert";
	case VK_DELETE:
		return "Delete";
	case VK_F1:
		return "F1";
	case VK_F2:
		return "F2";
	case VK_F3:
		return "F3";
	case VK_F4:
		return "F4";
	case VK_F5:
		return "F5";
	case VK_F6:
		return "F6";
	case VK_F7:
		return "F7";
	case VK_F8:
		return "F8";
	case VK_F9:
		return "F9";
	case VK_F10:
		return "F10";
	case VK_F11:
		return "F11";
	case VK_F12:
		return "F12";
	default: {
		UINT scanCode = MapVirtualKey(vkCode, MAPVK_VK_TO_VSC);
		char keyName[128];
		if (GetKeyNameTextA(scanCode << 16, keyName, sizeof(keyName)) > 0) {
			return std::string(keyName);
		} else {
			return "Unknown";
		}
	}
	}
}

std::string getCurrentCombination()
{
	std::vector<int> orderedKeys = {VK_CONTROL, VK_LCONTROL, VK_RCONTROL, VK_LWIN,   VK_RWIN,  VK_MENU,
					VK_LMENU,   VK_RMENU,    VK_SHIFT,    VK_LSHIFT, VK_RSHIFT};
	std::string combination;

	for (int key : orderedKeys) {
		if (pressedKeys.count(key)) {
			combination += getKeyName(key) + " + ";
		}
	}
	for (int key : pressedKeys) {
		if (modifierKeys.find(key) == modifierKeys.end()) {
			combination += getKeyName(key) + " + ";
		}
	}

	return combination.substr(0, combination.size() - 3); // Remove the trailing " + "
}

bool shouldLogCombination()
{
	// Ignore if only shift is the active modifier
	if (activeModifiers.size() == 1 &&
	    (activeModifiers.count(VK_SHIFT) > 0 || activeModifiers.count(VK_LSHIFT) > 0 || activeModifiers.count(VK_RSHIFT) > 0)) {
		return false;
	}
	return true;
}

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HC_ACTION) {
		KBDLLHOOKSTRUCT *p = (KBDLLHOOKSTRUCT *)lParam;
		if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
			pressedKeys.insert(p->vkCode);
			if (modifierKeys.count(p->vkCode)) {
				activeModifiers.insert(p->vkCode);
			}

			if ((pressedKeys.size() > 1 && isModifierKeyPressed() && shouldLogCombination()) ||
			    (singleKeys.count(p->vkCode) && !activeModifiers.count(VK_SHIFT) && !activeModifiers.count(VK_LSHIFT) &&
			     !activeModifiers.count(VK_RSHIFT))) {
				std::string keyCombination = getCurrentCombination();
				if (loggedCombinations.find(keyCombination) == loggedCombinations.end()) {
					blog(LOG_INFO, "[StreamUP Hotkey Display] Keys pressed: %s", keyCombination.c_str());
					if (hotkeyDisplayDock) {
						hotkeyDisplayDock->setLog(QString::fromStdString(keyCombination));
					}
					loggedCombinations.insert(keyCombination);

					// Emit the WebSocket event
					obs_data_t *event_data = obs_data_create();
					obs_data_set_string(event_data, "key_combination", keyCombination.c_str());
					obs_websocket_vendor_emit_event(websocket_vendor, "key_pressed", event_data);
					obs_data_release(event_data);
				}
			}
		} else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
			pressedKeys.erase(p->vkCode);
			if (modifierKeys.count(p->vkCode)) {
				activeModifiers.erase(p->vkCode);
			}

			if (!isModifierKeyPressed()) {
				loggedCombinations.clear();
			}
		}
	}
	return CallNextHookEx(keyboardHook, nCode, wParam, lParam);
}

void LoadHotkeyDisplayDock()
{
	const auto main_window = static_cast<QMainWindow *>(obs_frontend_get_main_window());
	obs_frontend_push_ui_translation(obs_module_get_string);

	hotkeyDisplayDock = new HotkeyDisplayDock(main_window);

	const QString title = QString::fromUtf8(obs_module_text("Hotkey Display Dock"));
	const auto name = "HotkeyDisplayDock";

#if LIBOBS_API_VER >= MAKE_SEMANTIC_VERSION(30, 0, 0)
	obs_frontend_add_dock_by_id(name, title.toUtf8().constData(), hotkeyDisplayDock);
#else
	auto dock = new QDockWidget(main_window);
	dock->setObjectName(name);
	dock->setWindowTitle(title);
	dock->setWidget(hotkeyDisplayDock);
	dock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
	dock->setFloating(true);
	dock->hide();
	obs_frontend_add_dock(dock);
#endif

	obs_frontend_pop_ui_translation();
}

obs_data_t *SaveLoadSettingsCallback(obs_data_t *save_data, bool saving)
{
	char *configPath = obs_module_config_path("configs.json");
	obs_data_t *data = nullptr;

	if (saving) {
		if (obs_data_save_json(save_data, configPath)) {
			blog(LOG_INFO, "[StreamUP Hotkey Display] Settings saved to %s", configPath);
		} else {
			blog(LOG_WARNING, "[StreamUP Hotkey Display] Failed to save settings to file.");
		}
	} else {
		data = obs_data_create_from_json_file(configPath);

		if (!data) {
			blog(LOG_INFO, "[StreamUP Hotkey Display] Settings not found. Creating settings file...");

			char *dirPath = obs_module_config_path("");
			os_mkdirs(dirPath);
			bfree(dirPath);

			data = obs_data_create();

			if (obs_data_save_json(data, configPath)) {
				blog(LOG_INFO, "[StreamUP Hotkey Display] Default settings saved to %s", configPath);
			} else {
				blog(LOG_WARNING, "[StreamUP Hotkey Display] Failed to save default settings to file.");
			}

			obs_data_release(data);
			data = obs_data_create_from_json_file(configPath);
		} else {
			blog(LOG_INFO, "[StreamUP Hotkey Display] Settings loaded successfully from %s", configPath);
		}
	}

	bfree(configPath);
	return data;
}

bool obs_module_load()
{
	blog(LOG_INFO, "[StreamUP Hotkey Display] loaded version %s", PROJECT_VERSION);

	// Initialize the WebSocket vendor
	websocket_vendor = obs_websocket_register_vendor("streamup-hotkey-display");
	if (!websocket_vendor) {
		blog(LOG_ERROR, "[StreamUP Hotkey Display] Failed to register websocket vendor!");
		return false;
	}

	// Load the hotkey display dock
	LoadHotkeyDisplayDock();

	// Load settings
	obs_data_t *settings = SaveLoadSettingsCallback(nullptr, false);

	if (settings) {
		if (hotkeyDisplayDock) {
			hotkeyDisplayDock->sceneName = QString::fromUtf8(obs_data_get_string(settings, "sceneName"));
			hotkeyDisplayDock->textSource = QString::fromUtf8(obs_data_get_string(settings, "textSource"));
			hotkeyDisplayDock->onScreenTime = obs_data_get_int(settings, "onScreenTime");
			hotkeyDisplayDock->prefix = QString::fromUtf8(obs_data_get_string(settings, "prefix"));
			hotkeyDisplayDock->suffix = QString::fromUtf8(obs_data_get_string(settings, "suffix"));

			// Initialize hookEnabled from settings
			bool hookEnabled = obs_data_get_bool(settings, "hookEnabled");
			hotkeyDisplayDock->setHookEnabled(hookEnabled);

			// Set the initial state of the hook and UI
			if (hookEnabled) {
				keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, NULL, 0);
				if (!keyboardHook) {
					blog(LOG_ERROR, "[StreamUP Hotkey Display] Failed to set keyboard hook!");
					hotkeyDisplayDock->setHookEnabled(false);
				}
				hotkeyDisplayDock->getToggleButton()->setText("Disable Hook");
				hotkeyDisplayDock->getLabel()->setStyleSheet("QLabel {"
									     "  border: 2px solid #4CAF50;"
									     "  padding: 10px;"
									     "  border-radius: 10px;"
									     "  font-size: 18px;"
									     "  color: #FFFFFF;"
									     "  background-color: #333333;"
									     "}");
			} else {
				hotkeyDisplayDock->getToggleButton()->setText("Enable Hook");
				hotkeyDisplayDock->getLabel()->setStyleSheet("QLabel {"
									     "  border: 2px solid #888888;"
									     "  padding: 10px;"
									     "  border-radius: 10px;"
									     "  font-size: 18px;"
									     "  color: #FFFFFF;"
									     "  background-color: #333333;"
									     "}");
			}
		}
		obs_data_release(settings);
	}

	return true;
}

void obs_module_unload()
{
	// Remove the keyboard hook
	if (keyboardHook) {
		UnhookWindowsHookEx(keyboardHook);
		keyboardHook = NULL;
	}

	// Unregister the WebSocket vendor
	if (websocket_vendor) {
		obs_websocket_vendor_unregister_request(websocket_vendor, "streamup_hotkey_display");
		websocket_vendor = nullptr;
	}
}

MODULE_EXPORT const char *obs_module_description(void)
{
	return obs_module_text("Description");
}

MODULE_EXPORT const char *obs_module_name(void)
{
	return obs_module_text("StreamUPHotkeyDisplay");
}
