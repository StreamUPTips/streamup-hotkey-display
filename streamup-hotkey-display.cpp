#include "streamup-hotkey-display-dock.hpp"
#include "streamup-hotkey-display-settings.hpp"
#include "version.h"
#include <obs-module.h>
#include <obs-frontend-api.h>
#include <obs.h>
#include <unordered_set>
#include <string>
#include <vector>
#include <QMainWindow>
#include <QDockWidget>
#include <util/platform.h>
#include "obs-websocket-api.h"

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef __APPLE__
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#endif

#ifdef __linux__
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif

#define QT_UTF8(str) QString::fromUtf8(str)
#define QT_TO_UTF8(str) str.toUtf8().constData()

OBS_DECLARE_MODULE()
OBS_MODULE_AUTHOR("Andilippi")
OBS_MODULE_USE_DEFAULT_LOCALE("streamup-hotkey-display", "en-US")

#ifdef _WIN32
HHOOK keyboardHook;
HHOOK mouseHook;
#endif

#ifdef __linux__
Display *display;
#endif

std::unordered_set<int> pressedKeys;
std::unordered_set<int> activeModifiers;

#ifdef _WIN32
std::unordered_set<int> modifierKeys = {VK_CONTROL, VK_LCONTROL, VK_RCONTROL, VK_MENU, VK_LMENU, VK_RMENU,
					VK_SHIFT,   VK_LSHIFT,   VK_RSHIFT,   VK_LWIN, VK_RWIN};

std::unordered_set<int> singleKeys = {VK_INSERT, VK_DELETE, VK_HOME, VK_END, VK_PRIOR, VK_NEXT, VK_F1,  VK_F2,  VK_F3,
				      VK_F4,     VK_F5,     VK_F6,   VK_F7,  VK_F8,    VK_F9,   VK_F10, VK_F11, VK_F12};

std::unordered_set<int> mouseButtons = {VK_LBUTTON, VK_RBUTTON, VK_MBUTTON, VK_XBUTTON1, VK_XBUTTON2};
std::unordered_set<int> scrollActions = {WM_MOUSEWHEEL, WM_MOUSEHWHEEL};
#endif

#ifdef __APPLE__
std::unordered_set<int> modifierKeys = {kVK_Control,      kVK_Command,      kVK_Option,      kVK_Shift,
					kVK_RightControl, kVK_RightCommand, kVK_RightOption, kVK_RightShift};

std::unordered_set<int> singleKeys = {
	kVK_ANSI_Keypad0, kVK_ANSI_Keypad1, kVK_ANSI_Keypad2, kVK_ANSI_Keypad3, kVK_ANSI_Keypad4,     kVK_ANSI_Keypad5,
	kVK_ANSI_Keypad6, kVK_ANSI_Keypad7, kVK_ANSI_Keypad8, kVK_ANSI_Keypad9, kVK_ANSI_KeypadClear, kVK_ANSI_KeypadEnter,
	kVK_Escape,       kVK_Delete,       kVK_Home,         kVK_End,          kVK_PageUp,           kVK_PageDown,
	kVK_Return};
#endif

#ifdef __linux__
std::unordered_set<int> modifierKeys = {XK_Control_L, XK_Control_R, XK_Super_L, XK_Super_R,
					XK_Alt_L,     XK_Alt_R,     XK_Shift_L, XK_Shift_R};

std::unordered_set<int> singleKeys = {XK_Insert, XK_Delete, XK_Home, XK_End, XK_Page_Up, XK_Page_Down, XK_F1,
				      XK_F2,     XK_F3,     XK_F4,   XK_F5,  XK_F6,      XK_F7,        XK_F8,
				      XK_F9,     XK_F10,    XK_F11,  XK_F12, XK_Return};
#endif

std::unordered_set<std::string> loggedCombinations;

HotkeyDisplayDock *hotkeyDisplayDock = nullptr;
StreamupHotkeyDisplaySettings *settingsDialog = nullptr;
obs_websocket_vendor websocket_vendor = nullptr;

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
#ifdef _WIN32
	switch (vkCode) {
	case VK_LBUTTON:
		return "Left Click";
	case VK_RBUTTON:
		return "Right Click";
	case VK_MBUTTON:
		return "Middle Click";
	case VK_XBUTTON1:
		return "X Button 1";
	case VK_XBUTTON2:
		return "X Button 2";
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
#endif

#ifdef __APPLE__
	switch (vkCode) {
	case kVK_Control:
	case kVK_RightControl:
		return "Ctrl";
	case kVK_Command:
	case kVK_RightCommand:
		return "Cmd";
	case kVK_Option:
	case kVK_RightOption:
		return "Alt";
	case kVK_Shift:
	case kVK_RightShift:
		return "Shift";
	case kVK_ANSI_KeypadEnter:
	case kVK_Return:
		return "Enter";
	case kVK_Space:
		return "Space";
	case kVK_Delete:
		return "Backspace";
	case kVK_Tab:
		return "Tab";
	case kVK_Escape:
		return "Escape";
	case kVK_PageUp:
		return "Page Up";
	case kVK_PageDown:
		return "Page Down";
	case kVK_End:
		return "End";
	case kVK_Home:
		return "Home";
	case kVK_LeftArrow:
		return "Left Arrow";
	case kVK_UpArrow:
		return "Up Arrow";
	case kVK_RightArrow:
		return "Right Arrow";
	case kVK_DownArrow:
		return "Down Arrow";
	case kVK_Help: // Use kVK_Help for Insert
		return "Insert";
	case kVK_F1:
		return "F1";
	case kVK_F2:
		return "F2";
	case kVK_F3:
		return "F3";
	case kVK_F4:
		return "F4";
	case kVK_F5:
		return "F5";
	case kVK_F6:
		return "F6";
	case kVK_F7:
		return "F7";
	case kVK_F8:
		return "F8";
	case kVK_F9:
		return "F9";
	case kVK_F10:
		return "F10";
	case kVK_F11:
		return "F11";
	case kVK_F12:
		return "F12";
	default: {
		return "Unknown";
	}
	}
#endif

#ifdef __linux__
	switch (vkCode) {
	case XK_Control_L:
	case XK_Control_R:
		return "Ctrl";
	case XK_Super_L:
	case XK_Super_R:
		return "Super";
	case XK_Alt_L:
	case XK_Alt_R:
		return "Alt";
	case XK_Shift_L:
	case XK_Shift_R:
		return "Shift";
	case XK_Return:
		return "Enter";
	case XK_space:
		return "Space";
	case XK_BackSpace:
		return "Backspace";
	case XK_Tab:
		return "Tab";
	case XK_Escape:
		return "Escape";
	case XK_Page_Up:
		return "Page Up";
	case XK_Page_Down:
		return "Page Down";
	case XK_End:
		return "End";
	case XK_Home:
		return "Home";
	case XK_Left:
		return "Left Arrow";
	case XK_Up:
		return "Up Arrow";
	case XK_Right:
		return "Right Arrow";
	case XK_Down:
		return "Down Arrow";
	case XK_Insert:
		return "Insert";
	case XK_Delete:
		return "Delete";
	case XK_F1:
		return "F1";
	case XK_F2:
		return "F2";
	case XK_F3:
		return "F3";
	case XK_F4:
		return "F4";
	case XK_F5:
		return "F5";
	case XK_F6:
		return "F6";
	case XK_F7:
		return "F7";
	case XK_F8:
		return "F8";
	case XK_F9:
		return "F9";
	case XK_F10:
		return "F10";
	case XK_F11:
		return "F11";
	case XK_F12:
		return "F12";
	default: {
		return "Unknown";
	}
	}
#endif
}

std::string getCurrentCombination()
{
	std::vector<int> orderedKeys;
#ifdef _WIN32
	orderedKeys = {VK_CONTROL, VK_LCONTROL, VK_RCONTROL, VK_LWIN,   VK_RWIN,  VK_MENU,
		       VK_LMENU,   VK_RMENU,    VK_SHIFT,    VK_LSHIFT, VK_RSHIFT};
#endif

#ifdef __APPLE__
	orderedKeys = {kVK_Control,      kVK_Command,      kVK_Option,      kVK_Shift,
		       kVK_RightControl, kVK_RightCommand, kVK_RightOption, kVK_RightShift};
#endif

#ifdef __linux__
	orderedKeys = {XK_Control_L, XK_Control_R, XK_Super_L, XK_Super_R, XK_Alt_L, XK_Alt_R, XK_Shift_L, XK_Shift_R};
#endif

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
#ifdef _WIN32
	if (activeModifiers.size() == 1 &&
	    (activeModifiers.count(VK_SHIFT) > 0 || activeModifiers.count(VK_LSHIFT) > 0 || activeModifiers.count(VK_RSHIFT) > 0)) {
		return false;
	}
#elif defined(__APPLE__)
	if (activeModifiers.size() == 1 && (activeModifiers.count(kVK_Shift) > 0 || activeModifiers.count(kVK_RightShift) > 0)) {
		return false;
	}
#elif defined(__linux__)
	if (activeModifiers.size() == 1 && (activeModifiers.count(XK_Shift_L) > 0 || activeModifiers.count(XK_Shift_R) > 0)) {
		return false;
	}
#endif
	return true;
}

#ifdef _WIN32
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

					// Add all key presses as an array
					obs_data_array_t *key_presses_array = obs_data_array_create();
					for (const int &key : pressedKeys) {
						obs_data_t *key_data = obs_data_create();
						obs_data_set_string(key_data, "key", getKeyName(key).c_str());
						obs_data_array_push_back(key_presses_array, key_data);
						obs_data_release(key_data);
					}
					obs_data_set_array(event_data, "key_presses", key_presses_array);
					obs_data_array_release(key_presses_array);

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

LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HC_ACTION) {
		MSLLHOOKSTRUCT *p = (MSLLHOOKSTRUCT *)lParam;

		// Only proceed if a modifier key is pressed
		if (isModifierKeyPressed()) {
			std::string keyCombination = getCurrentCombination(); // Get current key combination with any modifiers

			bool actionDetected = false;

			// Handle mouse button clicks
			switch (wParam) {
			case WM_LBUTTONDOWN:
				keyCombination += " + Left Click";
				actionDetected = true;
				break;
			case WM_RBUTTONDOWN:
				keyCombination += " + Right Click";
				actionDetected = true;
				break;
			case WM_MBUTTONDOWN:
				keyCombination += " + Middle Click";
				actionDetected = true;
				break;
			case WM_XBUTTONDOWN:
				if (HIWORD(p->mouseData) == XBUTTON1) {
					keyCombination += " + X Button 1";
				} else if (HIWORD(p->mouseData) == XBUTTON2) {
					keyCombination += " + X Button 2";
				}
				actionDetected = true;
				break;
			}

			// Handle scroll actions
			if (wParam == WM_MOUSEWHEEL) {
				if (GET_WHEEL_DELTA_WPARAM(p->mouseData) > 0)
					keyCombination += " + Scroll Up";
				else
					keyCombination += " + Scroll Down";
				actionDetected = true;
			} else if (wParam == WM_MOUSEHWHEEL) {
				if (GET_WHEEL_DELTA_WPARAM(p->mouseData) > 0)
					keyCombination += " + Scroll Right";
				else
					keyCombination += " + Scroll Left";
				actionDetected = true;
			}

			// Log and display the key combination if an action was detected
			if (actionDetected) {
				blog(LOG_INFO, "[StreamUP Hotkey Display] Mouse action detected: %s", keyCombination.c_str());
				if (hotkeyDisplayDock) {
					hotkeyDisplayDock->setLog(QString::fromStdString(keyCombination));
				}
			}
		}
	}
	return CallNextHookEx(mouseHook, nCode, wParam, lParam);
}

#endif

#ifdef __APPLE__
CFMachPortRef eventTap = nullptr;

CGEventRef CGEventCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *refcon);

void startMacOSKeyboardHook()
{
	eventTap = CGEventTapCreate(kCGSessionEventTap, kCGHeadInsertEventTap, kCGEventTapOptionDefault,
				    CGEventMaskBit(kCGEventKeyDown) | CGEventMaskBit(kCGEventKeyUp), CGEventCallback, nullptr);

	if (!eventTap) {
		blog(LOG_ERROR, "[StreamUP Hotkey Display] Failed to create event tap!");
		return;
	}

	CFRunLoopSourceRef runLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, eventTap, 0);
	CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopCommonModes);
	CGEventTapEnable(eventTap, true);
	CFRelease(runLoopSource);
}

void stopMacOSKeyboardHook()
{
	if (eventTap) {
		CFRelease(eventTap);
		eventTap = nullptr;
	}
}

CGEventRef CGEventCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *refcon)
{
	(void)proxy;
	(void)refcon;

	if (type == kCGEventKeyDown || type == kCGEventKeyUp) {
		CGKeyCode keyCode = (CGKeyCode)CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
		bool keyDown = (type == kCGEventKeyDown);

		if (keyDown) {
			pressedKeys.insert(keyCode);
			if (modifierKeys.count(keyCode)) {
				activeModifiers.insert(keyCode);
			}
		} else {
			pressedKeys.erase(keyCode);
			if (modifierKeys.count(keyCode)) {
				activeModifiers.erase(keyCode);
			}

			if (!isModifierKeyPressed()) {
				loggedCombinations.clear();
			}
		}

		if ((pressedKeys.size() > 1 && isModifierKeyPressed() && shouldLogCombination()) ||
		    (singleKeys.count(keyCode) && !activeModifiers.count(kVK_Shift) && !activeModifiers.count(kVK_RightShift))) {
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

				// Add all key presses as an array
				obs_data_array_t *key_presses_array = obs_data_array_create();
				for (const int &key : pressedKeys) {
					obs_data_t *key_data = obs_data_create();
					obs_data_set_string(key_data, "key", getKeyName(key).c_str());
					obs_data_array_push_back(key_presses_array, key_data);
					obs_data_release(key_data);
				}
				obs_data_set_array(event_data, "key_presses", key_presses_array);
				obs_data_array_release(key_presses_array);

				obs_websocket_vendor_emit_event(websocket_vendor, "key_pressed", event_data);
				obs_data_release(event_data);
			}
		}
	}
	return event;
}
#endif

#ifdef __linux__
void startLinuxKeyboardHook()
{
	display = XOpenDisplay(nullptr);
	if (!display) {
		blog(LOG_ERROR, "[StreamUP Hotkey Display] Failed to open X display!");
		return;
	}

	Window root = DefaultRootWindow(display);
	XSelectInput(display, root, KeyPressMask | KeyReleaseMask);

	XEvent event;
	while (true) {
		XNextEvent(display, &event);
		if (event.type == KeyPress) {
			KeySym keysym = XLookupKeysym(&event.xkey, 0);
			int keyCode = XKeysymToKeycode(display, keysym);
			pressedKeys.insert(keyCode);
			if (modifierKeys.count(keyCode)) {
				activeModifiers.insert(keyCode);
			}

			if ((pressedKeys.size() > 1 && isModifierKeyPressed() && shouldLogCombination()) ||
			    (singleKeys.count(keyCode) && !activeModifiers.count(XK_Shift_L) &&
			     !activeModifiers.count(XK_Shift_R))) {
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

					// Add all key presses as an array
					obs_data_array_t *key_presses_array = obs_data_array_create();
					for (const int &key : pressedKeys) {
						obs_data_t *key_data = obs_data_create();
						obs_data_set_string(key_data, "key", getKeyName(key).c_str());
						obs_data_array_push_back(key_presses_array, key_data);
						obs_data_release(key_data);
					}
					obs_data_set_array(event_data, "key_presses", key_presses_array);
					obs_data_array_release(key_presses_array);

					obs_websocket_vendor_emit_event(websocket_vendor, "key_pressed", event_data);
					obs_data_release(event_data);
				}
			}
		} else if (event.type == KeyRelease) {
			KeySym keysym = XLookupKeysym(&event.xkey, 0);
			int keyCode = XKeysymToKeycode(display, keysym);
			pressedKeys.erase(keyCode);
			if (modifierKeys.count(keyCode)) {
				activeModifiers.erase(keyCode);
			}

			if (!isModifierKeyPressed()) {
				loggedCombinations.clear();
			}
		}
	}
}

void stopLinuxKeyboardHook()
{
	if (display) {
		XCloseDisplay(display);
		display = nullptr;
	}
}
#endif

void LoadHotkeyDisplayDock()
{
	const auto main_window = static_cast<QMainWindow *>(obs_frontend_get_main_window());
	obs_frontend_push_ui_translation(obs_module_get_string);

	hotkeyDisplayDock = new HotkeyDisplayDock(main_window);

	const QString title = QString::fromUtf8(obs_module_text("StreamUP Hotkey Display Dock"));
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

	websocket_vendor = obs_websocket_register_vendor("streamup-hotkey-display");
	if (!websocket_vendor) {
		blog(LOG_ERROR, "[StreamUP Hotkey Display] Failed to register websocket vendor!");
		return false;
	}

	LoadHotkeyDisplayDock();

	obs_data_t *settings = SaveLoadSettingsCallback(nullptr, false);

	if (settings) {
		if (hotkeyDisplayDock) {
			hotkeyDisplayDock->sceneName = QString::fromUtf8(obs_data_get_string(settings, "sceneName"));
			hotkeyDisplayDock->textSource = QString::fromUtf8(obs_data_get_string(settings, "textSource"));
			hotkeyDisplayDock->onScreenTime = obs_data_get_int(settings, "onScreenTime");
			hotkeyDisplayDock->prefix = QString::fromUtf8(obs_data_get_string(settings, "prefix"));
			hotkeyDisplayDock->suffix = QString::fromUtf8(obs_data_get_string(settings, "suffix"));
			hotkeyDisplayDock->setDisplayInTextSource(obs_data_get_bool(settings, "displayInTextSource"));

			if (hotkeyDisplayDock->sceneName.isEmpty()) {
				hotkeyDisplayDock->sceneName = "Default Scene";
			}
			if (hotkeyDisplayDock->textSource.isEmpty()) {
				hotkeyDisplayDock->textSource = "Default Text Source";
			}
			if (hotkeyDisplayDock->onScreenTime == 0) {
				hotkeyDisplayDock->onScreenTime = 100;
			}
			if (hotkeyDisplayDock->prefix.isEmpty()) {
				hotkeyDisplayDock->prefix = "";
			}
			if (hotkeyDisplayDock->suffix.isEmpty()) {
				hotkeyDisplayDock->suffix = "";
			}

			bool hookEnabled = obs_data_get_bool(settings, "hookEnabled");
			hotkeyDisplayDock->setHookEnabled(hookEnabled);

			// Set the button text based on the hook status
			if (hookEnabled) {
				hotkeyDisplayDock->getToggleButton()->setText(obs_module_text("DisableHookButton"));
				hotkeyDisplayDock->getLabel()->setStyleSheet("QLabel {"
									     "  border: 2px solid #4CAF50;"
									     "  padding: 10px;"
									     "  border-radius: 10px;"
									     "  font-size: 18px;"
									     "  color: #FFFFFF;"
									     "  background-color: #333333;"
									     "}");
			} else {
				hotkeyDisplayDock->getToggleButton()->setText(obs_module_text("EnableHookButton"));
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
	} else {
		if (hotkeyDisplayDock) {
			hotkeyDisplayDock->sceneName = "Default Scene";
			hotkeyDisplayDock->textSource = "Default Text Source";
			hotkeyDisplayDock->onScreenTime = 100;
			hotkeyDisplayDock->prefix = "";
			hotkeyDisplayDock->suffix = "";
			hotkeyDisplayDock->setDisplayInTextSource(false);

			// Set the button text to the default state
			hotkeyDisplayDock->getToggleButton()->setText(obs_module_text("EnableHookButton"));
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

	return true;
}

void obs_module_unload()
{
#ifdef _WIN32
	if (keyboardHook) {
		UnhookWindowsHookEx(keyboardHook);
		keyboardHook = NULL;
	}
#elif defined(__APPLE__)
	stopMacOSKeyboardHook();
#elif defined(__linux__)
	stopLinuxKeyboardHook();
#endif

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
