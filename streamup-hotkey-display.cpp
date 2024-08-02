#include "streamup-hotkey-display.hpp"
#include "version.h"
#include <obs.h>
#include <obs-data.h>
#include <obs-module.h>
#include <util/platform.h>
#include <windows.h>
#include <unordered_set>
#include <string>
#include <vector>

#define QT_UTF8(str) QString::fromUtf8(str)
#define QT_TO_UTF8(str) str.toUtf8().constData()

OBS_DECLARE_MODULE()
OBS_MODULE_AUTHOR("Andilippi");
OBS_MODULE_USE_DEFAULT_LOCALE("streamup-hotkey-display", "en-US")

HHOOK keyboardHook;
std::unordered_set<int> pressedKeys;
std::unordered_set<int> activeModifiers;
std::unordered_set<int> modifierKeys = {VK_CONTROL, VK_LCONTROL, VK_RCONTROL, VK_MENU, VK_LMENU, VK_RMENU,
					VK_SHIFT,   VK_LSHIFT,   VK_RSHIFT,   VK_LWIN, VK_RWIN};

std::unordered_set<std::string> loggedCombinations;

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
	if (activeModifiers.size() == 0 || (activeModifiers.size() == 1 && activeModifiers.count(VK_SHIFT) > 0)) {
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

			if (pressedKeys.size() > 1 && isModifierKeyPressed() && shouldLogCombination()) {
				std::string keyCombination = getCurrentCombination();
				if (loggedCombinations.find(keyCombination) == loggedCombinations.end()) {
					blog(LOG_INFO, "[StreamUP Hotkey Display] Keys pressed: %s", keyCombination.c_str());
					loggedCombinations.insert(keyCombination);
				}
			}
		} else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
			pressedKeys.erase(p->vkCode);
			if (modifierKeys.count(p->vkCode)) {
				activeModifiers.erase(p->vkCode);
			}

			if (!isModifierKeyPressed()) {
				loggedCombinations.clear(); // Clear logged combinations when no modifiers are held
			}
		}
	}
	return CallNextHookEx(keyboardHook, nCode, wParam, lParam);
}

bool obs_module_load()
{
	blog(LOG_INFO, "[StreamUP Hotkey Display] loaded version %s", PROJECT_VERSION);

	// Set the keyboard hook
	keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, NULL, 0);
	if (!keyboardHook) {
		blog(LOG_ERROR, "[StreamUP Hotkey Display] Failed to set keyboard hook!");
		return false;
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
}

MODULE_EXPORT const char *obs_module_description(void)
{
	return obs_module_text("Description");
}

MODULE_EXPORT const char *obs_module_name(void)
{
	return obs_module_text("StreamUPHotkeyDisplay");
}
