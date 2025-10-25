#include "streamup-hotkey-display-dock.hpp"
#include "streamup-hotkey-display-settings.hpp"
#include "version.h"
#include <obs-module.h>
#include <obs-frontend-api.h>
#include <obs.h>
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <vector>
#include <mutex>
#include <algorithm>
#include <thread>
#include <atomic>
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
std::thread linuxHookThread;
std::atomic<bool> linuxHookRunning{false};
#endif

std::unordered_set<int> pressedKeys;
std::unordered_set<int> activeModifiers;
std::mutex keyStateMutex; // Protects pressedKeys, activeModifiers, and loggedCombinations

#ifdef _WIN32
std::unordered_set<int> modifierKeys = {VK_CONTROL, VK_LCONTROL, VK_RCONTROL, VK_MENU, VK_LMENU, VK_RMENU,
					VK_SHIFT,   VK_LSHIFT,   VK_RSHIFT,   VK_LWIN, VK_RWIN};

std::unordered_set<int> singleKeys = {VK_INSERT, VK_DELETE, VK_HOME, VK_END, VK_PRIOR, VK_NEXT, VK_F1,  VK_F2,  VK_F3,
				      VK_F4,     VK_F5,     VK_F6,   VK_F7,  VK_F8,    VK_F9,   VK_F10, VK_F11, VK_F12};

std::unordered_set<int> mouseButtons = {VK_LBUTTON, VK_RBUTTON, VK_MBUTTON, VK_XBUTTON1, VK_XBUTTON2};
std::unordered_set<int> scrollActions = {WM_MOUSEWHEEL, WM_MOUSEHWHEEL};

// Additional key categories for single key capture
std::unordered_set<int> numpadKeys = {VK_NUMPAD0, VK_NUMPAD1, VK_NUMPAD2, VK_NUMPAD3, VK_NUMPAD4,
				      VK_NUMPAD5, VK_NUMPAD6, VK_NUMPAD7, VK_NUMPAD8, VK_NUMPAD9,
				      VK_MULTIPLY, VK_ADD, VK_SEPARATOR, VK_SUBTRACT, VK_DECIMAL, VK_DIVIDE};

std::unordered_set<int> numberKeys = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};

std::unordered_set<int> letterKeys = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
				      'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'};

std::unordered_set<int> punctuationKeys = {VK_OEM_1,      VK_OEM_PLUS,   VK_OEM_COMMA, VK_OEM_MINUS, VK_OEM_PERIOD,
					   VK_OEM_2,      VK_OEM_3,      VK_OEM_4,     VK_OEM_5,     VK_OEM_6,
					   VK_OEM_7,      VK_OEM_102,    VK_SPACE,     VK_TAB,       VK_OEM_8,
					   VK_OEM_AX,     VK_OEM_CLEAR,  VK_BACK};
#endif

#ifdef __APPLE__
std::unordered_set<int> modifierKeys = {kVK_Control,      kVK_Command,      kVK_Option,      kVK_Shift,
					kVK_RightControl, kVK_RightCommand, kVK_RightOption, kVK_RightShift};

std::unordered_set<int> singleKeys = {
	kVK_ANSI_Keypad0, kVK_ANSI_Keypad1, kVK_ANSI_Keypad2, kVK_ANSI_Keypad3, kVK_ANSI_Keypad4,     kVK_ANSI_Keypad5,
	kVK_ANSI_Keypad6, kVK_ANSI_Keypad7, kVK_ANSI_Keypad8, kVK_ANSI_Keypad9, kVK_ANSI_KeypadClear, kVK_ANSI_KeypadEnter,
	kVK_Escape,       kVK_Delete,       kVK_Home,         kVK_End,          kVK_PageUp,           kVK_PageDown,
	kVK_Return};

// Additional key categories for single key capture
std::unordered_set<int> numpadKeys = {kVK_ANSI_Keypad0,     kVK_ANSI_Keypad1,    kVK_ANSI_Keypad2,
				      kVK_ANSI_Keypad3,     kVK_ANSI_Keypad4,    kVK_ANSI_Keypad5,
				      kVK_ANSI_Keypad6,     kVK_ANSI_Keypad7,    kVK_ANSI_Keypad8,
				      kVK_ANSI_Keypad9,     kVK_ANSI_KeypadClear, kVK_ANSI_KeypadEnter,
				      kVK_ANSI_KeypadPlus,  kVK_ANSI_KeypadMinus, kVK_ANSI_KeypadMultiply,
				      kVK_ANSI_KeypadDivide, kVK_ANSI_KeypadDecimal, kVK_ANSI_KeypadEquals};

std::unordered_set<int> numberKeys = {kVK_ANSI_0, kVK_ANSI_1, kVK_ANSI_2, kVK_ANSI_3, kVK_ANSI_4,
				      kVK_ANSI_5, kVK_ANSI_6, kVK_ANSI_7, kVK_ANSI_8, kVK_ANSI_9};

std::unordered_set<int> letterKeys = {kVK_ANSI_A, kVK_ANSI_B, kVK_ANSI_C, kVK_ANSI_D, kVK_ANSI_E, kVK_ANSI_F,
				      kVK_ANSI_G, kVK_ANSI_H, kVK_ANSI_I, kVK_ANSI_J, kVK_ANSI_K, kVK_ANSI_L,
				      kVK_ANSI_M, kVK_ANSI_N, kVK_ANSI_O, kVK_ANSI_P, kVK_ANSI_Q, kVK_ANSI_R,
				      kVK_ANSI_S, kVK_ANSI_T, kVK_ANSI_U, kVK_ANSI_V, kVK_ANSI_W, kVK_ANSI_X,
				      kVK_ANSI_Y, kVK_ANSI_Z};

std::unordered_set<int> punctuationKeys = {
	kVK_ANSI_Semicolon,     kVK_ANSI_Quote,        kVK_ANSI_Comma,       kVK_ANSI_Period,
	kVK_ANSI_Slash,         kVK_ANSI_Backslash,    kVK_ANSI_LeftBracket, kVK_ANSI_RightBracket,
	kVK_ANSI_Grave,         kVK_ANSI_Equal,        kVK_ANSI_Minus,       kVK_Space,
	kVK_Tab,                kVK_Delete,            kVK_ForwardDelete};
#endif

#ifdef __linux__
std::unordered_set<int> modifierKeys = {XK_Control_L, XK_Control_R, XK_Super_L, XK_Super_R,
					XK_Alt_L,     XK_Alt_R,     XK_Shift_L, XK_Shift_R};

std::unordered_set<int> singleKeys = {XK_Insert, XK_Delete, XK_Home, XK_End, XK_Page_Up, XK_Page_Down, XK_F1,
				      XK_F2,     XK_F3,     XK_F4,   XK_F5,  XK_F6,      XK_F7,        XK_F8,
				      XK_F9,     XK_F10,    XK_F11,  XK_F12, XK_Return};

// Additional key categories for single key capture
std::unordered_set<int> numpadKeys = {XK_KP_0,        XK_KP_1,      XK_KP_2,       XK_KP_3,
				      XK_KP_4,        XK_KP_5,      XK_KP_6,       XK_KP_7,
				      XK_KP_8,        XK_KP_9,      XK_KP_Add,     XK_KP_Subtract,
				      XK_KP_Multiply, XK_KP_Divide, XK_KP_Decimal, XK_KP_Enter};

std::unordered_set<int> numberKeys = {XK_0, XK_1, XK_2, XK_3, XK_4, XK_5, XK_6, XK_7, XK_8, XK_9};

std::unordered_set<int> letterKeys = {XK_a, XK_b, XK_c, XK_d, XK_e, XK_f, XK_g, XK_h, XK_i,
				      XK_j, XK_k, XK_l, XK_m, XK_n, XK_o, XK_p, XK_q, XK_r,
				      XK_s, XK_t, XK_u, XK_v, XK_w, XK_x, XK_y, XK_z,
				      XK_A, XK_B, XK_C, XK_D, XK_E, XK_F, XK_G, XK_H, XK_I,
				      XK_J, XK_K, XK_L, XK_M, XK_N, XK_O, XK_P, XK_Q, XK_R,
				      XK_S, XK_T, XK_U, XK_V, XK_W, XK_X, XK_Y, XK_Z};

std::unordered_set<int> punctuationKeys = {
	XK_semicolon, XK_comma,     XK_period,    XK_slash,      XK_backslash,   XK_apostrophe,
	XK_grave,     XK_bracketleft, XK_bracketright, XK_equal,      XK_minus,       XK_space,
	XK_Tab,       XK_BackSpace, XK_Return};
#endif

std::unordered_set<std::string> loggedCombinations;

// Single key capture settings
bool captureNumpad = false;
bool captureNumbers = false;
bool captureLetters = false;
bool capturePunctuation = false;
std::unordered_set<int> whitelistedKeySet;

HotkeyDisplayDock *hotkeyDisplayDock = nullptr;
StreamupHotkeyDisplaySettings *settingsDialog = nullptr;
obs_websocket_vendor websocket_vendor = nullptr;

// Key name lookup tables for performance
#ifdef _WIN32
static const std::unordered_map<int, const char *> keyNameMap = {
	{VK_LBUTTON, "Left Click"},  {VK_RBUTTON, "Right Click"},  {VK_MBUTTON, "Middle Click"},
	{VK_XBUTTON1, "X Button 1"}, {VK_XBUTTON2, "X Button 2"},  {VK_CONTROL, "Ctrl"},
	{VK_LCONTROL, "Ctrl"},       {VK_RCONTROL, "Ctrl"},        {VK_MENU, "Alt"},
	{VK_LMENU, "Alt"},           {VK_RMENU, "Alt"},            {VK_SHIFT, "Shift"},
	{VK_LSHIFT, "Shift"},        {VK_RSHIFT, "Shift"},         {VK_LWIN, "Win"},
	{VK_RWIN, "Win"},            {VK_RETURN, "Enter"},         {VK_SPACE, "Space"},
	{VK_BACK, "Backspace"},      {VK_TAB, "Tab"},              {VK_ESCAPE, "Escape"},
	{VK_PRIOR, "Page Up"},       {VK_NEXT, "Page Down"},       {VK_END, "End"},
	{VK_HOME, "Home"},           {VK_LEFT, "Left Arrow"},      {VK_UP, "Up Arrow"},
	{VK_RIGHT, "Right Arrow"},   {VK_DOWN, "Down Arrow"},      {VK_INSERT, "Insert"},
	{VK_DELETE, "Delete"},       {VK_F1, "F1"},                {VK_F2, "F2"},
	{VK_F3, "F3"},               {VK_F4, "F4"},                {VK_F5, "F5"},
	{VK_F6, "F6"},               {VK_F7, "F7"},                {VK_F8, "F8"},
	{VK_F9, "F9"},               {VK_F10, "F10"},              {VK_F11, "F11"},
	{VK_F12, "F12"}};
#endif

#ifdef __APPLE__
static const std::unordered_map<int, const char *> keyNameMap = {
	{kVK_Control, "Ctrl"},          {kVK_RightControl, "Ctrl"},   {kVK_Command, "Cmd"},
	{kVK_RightCommand, "Cmd"},      {kVK_Option, "Alt"},          {kVK_RightOption, "Alt"},
	{kVK_Shift, "Shift"},           {kVK_RightShift, "Shift"},    {kVK_ANSI_KeypadEnter, "Enter"},
	{kVK_Return, "Enter"},          {kVK_Space, "Space"},         {kVK_Delete, "Backspace"},
	{kVK_Tab, "Tab"},               {kVK_Escape, "Escape"},       {kVK_PageUp, "Page Up"},
	{kVK_PageDown, "Page Down"},    {kVK_End, "End"},             {kVK_Home, "Home"},
	{kVK_LeftArrow, "Left Arrow"},  {kVK_UpArrow, "Up Arrow"},    {kVK_RightArrow, "Right Arrow"},
	{kVK_DownArrow, "Down Arrow"},  {kVK_Help, "Insert"},         {kVK_F1, "F1"},
	{kVK_F2, "F2"},                 {kVK_F3, "F3"},               {kVK_F4, "F4"},
	{kVK_F5, "F5"},                 {kVK_F6, "F6"},               {kVK_F7, "F7"},
	{kVK_F8, "F8"},                 {kVK_F9, "F9"},               {kVK_F10, "F10"},
	{kVK_F11, "F11"},               {kVK_F12, "F12"}};
#endif

#ifdef __linux__
static const std::unordered_map<int, const char *> keyNameMap = {
	{XK_Control_L, "Ctrl"},    {XK_Control_R, "Ctrl"},   {XK_Super_L, "Super"},
	{XK_Super_R, "Super"},     {XK_Alt_L, "Alt"},        {XK_Alt_R, "Alt"},
	{XK_Shift_L, "Shift"},     {XK_Shift_R, "Shift"},    {XK_Return, "Enter"},
	{XK_space, "Space"},       {XK_BackSpace, "Backspace"}, {XK_Tab, "Tab"},
	{XK_Escape, "Escape"},     {XK_Page_Up, "Page Up"},  {XK_Page_Down, "Page Down"},
	{XK_End, "End"},           {XK_Home, "Home"},        {XK_Left, "Left Arrow"},
	{XK_Up, "Up Arrow"},       {XK_Right, "Right Arrow"}, {XK_Down, "Down Arrow"},
	{XK_Insert, "Insert"},     {XK_Delete, "Delete"},    {XK_F1, "F1"},
	{XK_F2, "F2"},             {XK_F3, "F3"},            {XK_F4, "F4"},
	{XK_F5, "F5"},             {XK_F6, "F6"},            {XK_F7, "F7"},
	{XK_F8, "F8"},             {XK_F9, "F9"},            {XK_F10, "F10"},
	{XK_F11, "F11"},           {XK_F12, "F12"}};
#endif

bool isModifierKeyPressed()
{
	std::lock_guard<std::mutex> lock(keyStateMutex);
	for (const int key : activeModifiers) {
		if (pressedKeys.count(key)) {
			return true;
		}
	}
	return false;
}

std::string getKeyName(int vkCode)
{
	// Try lookup table first (O(1) average case)
	auto it = keyNameMap.find(vkCode);
	if (it != keyNameMap.end()) {
		return it->second;
	}

	// Fallback for keys not in lookup table
#ifdef _WIN32
	UINT scanCode = MapVirtualKey(vkCode, MAPVK_VK_TO_VSC);
	char keyName[128];
	if (GetKeyNameTextA(scanCode << 16, keyName, sizeof(keyName)) > 0) {
		return std::string(keyName);
	}
#endif

	return "Unknown";
}

std::string getCurrentCombination()
{
	std::lock_guard<std::mutex> lock(keyStateMutex);

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

	std::vector<std::string> keys;
	keys.reserve(pressedKeys.size()); // Pre-allocate space for efficiency

	// Add modifier keys in order
	for (const int key : orderedKeys) {
		if (pressedKeys.count(key)) {
			keys.push_back(getKeyName(key));
		}
	}

	// Add non-modifier keys
	for (const int key : pressedKeys) {
		if (modifierKeys.find(key) == modifierKeys.end()) {
			keys.push_back(getKeyName(key));
		}
	}

	// Join keys with " + " separator
	std::string combination;
	if (!keys.empty()) {
		combination = keys[0];
		for (size_t i = 1; i < keys.size(); ++i) {
			combination += " + " + keys[i];
		}
	}

	return combination;
}

bool shouldCaptureSingleKey(int keyCode)
{
	// Check if key is in the whitelist
	if (whitelistedKeySet.count(keyCode) > 0) {
		return true;
	}

	// Check category-based capture settings
	if (captureNumpad && numpadKeys.count(keyCode) > 0) {
		return true;
	}
	if (captureNumbers && numberKeys.count(keyCode) > 0) {
		return true;
	}
	if (captureLetters && letterKeys.count(keyCode) > 0) {
		return true;
	}
	if (capturePunctuation && punctuationKeys.count(keyCode) > 0) {
		return true;
	}

	// Check if it's in the default singleKeys set (F1-F12, Insert, Delete, etc.)
	if (singleKeys.count(keyCode) > 0) {
		return true;
	}

	return false;
}

bool shouldLogCombination()
{
	std::lock_guard<std::mutex> lock(keyStateMutex);
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

void emitWebSocketEvent(const std::string &keyCombination)
{
	if (!websocket_vendor) {
		return;
	}

	obs_data_t *event_data = obs_data_create();
	obs_data_set_string(event_data, "key_combination", keyCombination.c_str());

	// Add all key presses as an array
	obs_data_array_t *key_presses_array = obs_data_array_create();
	{
		std::lock_guard<std::mutex> lock(keyStateMutex);
		for (const int key : pressedKeys) {
			obs_data_t *key_data = obs_data_create();
			obs_data_set_string(key_data, "key", getKeyName(key).c_str());
			obs_data_array_push_back(key_presses_array, key_data);
			obs_data_release(key_data);
		}
	}
	obs_data_set_array(event_data, "key_presses", key_presses_array);
	obs_data_array_release(key_presses_array);

	obs_websocket_vendor_emit_event(websocket_vendor, "key_pressed", event_data);
	obs_data_release(event_data);
}

#ifdef _WIN32
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HC_ACTION) {
		KBDLLHOOKSTRUCT *p = (KBDLLHOOKSTRUCT *)lParam;
		if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
			{
				std::lock_guard<std::mutex> lock(keyStateMutex);
				pressedKeys.insert(p->vkCode);
				if (modifierKeys.count(p->vkCode)) {
					activeModifiers.insert(p->vkCode);
				}
			}

			if ((pressedKeys.size() > 1 && isModifierKeyPressed() && shouldLogCombination()) ||
			    (shouldCaptureSingleKey(p->vkCode) && !activeModifiers.count(VK_SHIFT) && !activeModifiers.count(VK_LSHIFT) &&
			     !activeModifiers.count(VK_RSHIFT))) {
				std::string keyCombination = getCurrentCombination();
				bool shouldLog = false;
				{
					std::lock_guard<std::mutex> lock(keyStateMutex);
					if (loggedCombinations.find(keyCombination) == loggedCombinations.end()) {
						loggedCombinations.insert(keyCombination);
						shouldLog = true;
					}
				}
				if (shouldLog) {
					blog(LOG_INFO, "[StreamUP Hotkey Display] Keys pressed: %s", keyCombination.c_str());
					if (hotkeyDisplayDock) {
						hotkeyDisplayDock->setLog(QString::fromStdString(keyCombination));
					}
					emitWebSocketEvent(keyCombination);
				}
			}
		} else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
			bool shouldClearCombinations = false;
			{
				std::lock_guard<std::mutex> lock(keyStateMutex);
				pressedKeys.erase(p->vkCode);
				if (modifierKeys.count(p->vkCode)) {
					activeModifiers.erase(p->vkCode);
				}

				// Check if we should clear combinations while holding the lock
				if (activeModifiers.empty() ||
				    std::none_of(activeModifiers.begin(), activeModifiers.end(),
				                 [](int key) { return pressedKeys.count(key) > 0; })) {
					loggedCombinations.clear();
				}
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

bool checkMacOSAccessibilityPermissions()
{
	// Check if we have accessibility permissions
	bool hasPermission = AXIsProcessTrusted();

	if (!hasPermission) {
		blog(LOG_WARNING, "[StreamUP Hotkey Display] Accessibility permissions not granted!");
		blog(LOG_WARNING, "[StreamUP Hotkey Display] Please grant Accessibility permissions in System Preferences:");
		blog(LOG_WARNING, "[StreamUP Hotkey Display]   System Preferences -> Security & Privacy -> Privacy -> Accessibility");
		blog(LOG_WARNING, "[StreamUP Hotkey Display]   Add OBS to the list and check the checkbox");
	}

	return hasPermission;
}

void startMacOSKeyboardHook()
{
	// Check permissions first
	if (!checkMacOSAccessibilityPermissions()) {
		blog(LOG_ERROR, "[StreamUP Hotkey Display] Cannot start keyboard hook without Accessibility permissions");

		// Show user-friendly error in the dock
		if (hotkeyDisplayDock) {
			QString errorTitle = QString::fromUtf8(obs_module_text("Error.Accessibility.Permission"));
			QString errorInstructions = QString::fromUtf8(obs_module_text("Error.Accessibility.Instructions"));
			hotkeyDisplayDock->setLog(errorTitle + "\n\n" + errorInstructions);
		}
		return;
	}

	// Create event tap for keyboard AND mouse events
	CGEventMask eventMask = CGEventMaskBit(kCGEventKeyDown) | CGEventMaskBit(kCGEventKeyUp) |
				CGEventMaskBit(kCGEventLeftMouseDown) | CGEventMaskBit(kCGEventRightMouseDown) |
				CGEventMaskBit(kCGEventOtherMouseDown) | CGEventMaskBit(kCGEventScrollWheel);

	eventTap = CGEventTapCreate(kCGSessionEventTap, kCGHeadInsertEventTap, kCGEventTapOptionDefault,
				    eventMask, CGEventCallback, nullptr);

	if (!eventTap) {
		blog(LOG_ERROR, "[StreamUP Hotkey Display] Failed to create event tap!");
		if (hotkeyDisplayDock) {
			hotkeyDisplayDock->setLog(QString::fromUtf8(obs_module_text("Error.EventTap.Failed")));
		}
		return;
	}

	CFRunLoopSourceRef runLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, eventTap, 0);
	CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopCommonModes);
	CGEventTapEnable(eventTap, true);
	CFRelease(runLoopSource);

	blog(LOG_INFO, "[StreamUP Hotkey Display] macOS keyboard and mouse hook started successfully");
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

	// Handle keyboard events
	if (type == kCGEventKeyDown || type == kCGEventKeyUp) {
		CGKeyCode keyCode = (CGKeyCode)CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
		bool keyDown = (type == kCGEventKeyDown);

		if (keyDown) {
			{
				std::lock_guard<std::mutex> lock(keyStateMutex);
				pressedKeys.insert(keyCode);
				if (modifierKeys.count(keyCode)) {
					activeModifiers.insert(keyCode);
				}
			}
		} else {
			{
				std::lock_guard<std::mutex> lock(keyStateMutex);
				pressedKeys.erase(keyCode);
				if (modifierKeys.count(keyCode)) {
					activeModifiers.erase(keyCode);
				}

				// Check if we should clear combinations while holding the lock
				if (activeModifiers.empty() ||
				    std::none_of(activeModifiers.begin(), activeModifiers.end(),
				                 [](int key) { return pressedKeys.count(key) > 0; })) {
					loggedCombinations.clear();
				}
			}
		}

		if ((pressedKeys.size() > 1 && isModifierKeyPressed() && shouldLogCombination()) ||
		    (shouldCaptureSingleKey(keyCode) && !activeModifiers.count(kVK_Shift) && !activeModifiers.count(kVK_RightShift))) {
			std::string keyCombination = getCurrentCombination();
			bool shouldLog = false;
			{
				std::lock_guard<std::mutex> lock(keyStateMutex);
				if (loggedCombinations.find(keyCombination) == loggedCombinations.end()) {
					loggedCombinations.insert(keyCombination);
					shouldLog = true;
				}
			}
			if (shouldLog) {
				blog(LOG_INFO, "[StreamUP Hotkey Display] Keys pressed: %s", keyCombination.c_str());
				if (hotkeyDisplayDock) {
					hotkeyDisplayDock->setLog(QString::fromStdString(keyCombination));
				}
				emitWebSocketEvent(keyCombination);
			}
		}
	}
	// Handle mouse events (only when modifier keys are pressed)
	else if (type == kCGEventLeftMouseDown || type == kCGEventRightMouseDown ||
	         type == kCGEventOtherMouseDown || type == kCGEventScrollWheel) {
		if (isModifierKeyPressed()) {
			std::string keyCombination = getCurrentCombination();

			// Add mouse action to combination
			if (type == kCGEventLeftMouseDown) {
				keyCombination += " + Left Click";
			} else if (type == kCGEventRightMouseDown) {
				keyCombination += " + Right Click";
			} else if (type == kCGEventOtherMouseDown) {
				int64_t buttonNumber = CGEventGetIntegerValueField(event, kCGMouseEventButtonNumber);
				if (buttonNumber == 2) {
					keyCombination += " + Middle Click";
				} else {
					keyCombination += " + Button " + std::to_string(buttonNumber + 1);
				}
			} else if (type == kCGEventScrollWheel) {
				int64_t deltaY = CGEventGetIntegerValueField(event, kCGScrollWheelEventDeltaAxis1);
				int64_t deltaX = CGEventGetIntegerValueField(event, kCGScrollWheelEventDeltaAxis2);

				if (deltaY > 0) {
					keyCombination += " + Scroll Up";
				} else if (deltaY < 0) {
					keyCombination += " + Scroll Down";
				} else if (deltaX > 0) {
					keyCombination += " + Scroll Right";
				} else if (deltaX < 0) {
					keyCombination += " + Scroll Left";
				}
			}

			blog(LOG_INFO, "[StreamUP Hotkey Display] Mouse action detected: %s", keyCombination.c_str());
			if (hotkeyDisplayDock) {
				hotkeyDisplayDock->setLog(QString::fromStdString(keyCombination));
			}
		}
	}
	return event;
}
#endif

#ifdef __linux__
void linuxKeyboardHookThreadFunc()
{
	display = XOpenDisplay(nullptr);
	if (!display) {
		blog(LOG_ERROR, "[StreamUP Hotkey Display] Failed to open X display!");
		linuxHookRunning = false;
		return;
	}

	Window root = DefaultRootWindow(display);
	XSelectInput(display, root, KeyPressMask | KeyReleaseMask | ButtonPressMask);

	// Set up non-blocking event checking
	int fd = ConnectionNumber(display);
	fd_set readfds;
	struct timeval tv;

	blog(LOG_INFO, "[StreamUP Hotkey Display] Linux keyboard hook thread started");

	XEvent event;
	while (linuxHookRunning) {
		// Use select to check for events with timeout
		FD_ZERO(&readfds);
		FD_SET(fd, &readfds);
		tv.tv_sec = 0;
		tv.tv_usec = 100000; // 100ms timeout

		int result = select(fd + 1, &readfds, nullptr, nullptr, &tv);
		if (result < 0) {
			blog(LOG_ERROR, "[StreamUP Hotkey Display] select() error in X11 event loop");
			break;
		}

		if (result == 0) {
			// Timeout - check if we should continue running
			continue;
		}

		// Process all pending events
		while (linuxHookRunning && XPending(display)) {
			XNextEvent(display, &event);
			if (event.type == KeyPress) {
				KeySym keysym = XLookupKeysym(&event.xkey, 0);
				int keyCode = XKeysymToKeycode(display, keysym);
				{
					std::lock_guard<std::mutex> lock(keyStateMutex);
					pressedKeys.insert(keyCode);
					if (modifierKeys.count(keyCode)) {
						activeModifiers.insert(keyCode);
					}
				}

				if ((pressedKeys.size() > 1 && isModifierKeyPressed() && shouldLogCombination()) ||
				    (shouldCaptureSingleKey(keyCode) && !activeModifiers.count(XK_Shift_L) &&
				     !activeModifiers.count(XK_Shift_R))) {
					std::string keyCombination = getCurrentCombination();
					bool shouldLog = false;
					{
						std::lock_guard<std::mutex> lock(keyStateMutex);
						if (loggedCombinations.find(keyCombination) == loggedCombinations.end()) {
							loggedCombinations.insert(keyCombination);
							shouldLog = true;
						}
					}
					if (shouldLog) {
						blog(LOG_INFO, "[StreamUP Hotkey Display] Keys pressed: %s", keyCombination.c_str());
						if (hotkeyDisplayDock) {
							hotkeyDisplayDock->setLog(QString::fromStdString(keyCombination));
						}
						emitWebSocketEvent(keyCombination);
					}
				}
			} else if (event.type == KeyRelease) {
				KeySym keysym = XLookupKeysym(&event.xkey, 0);
				int keyCode = XKeysymToKeycode(display, keysym);
				{
					std::lock_guard<std::mutex> lock(keyStateMutex);
					pressedKeys.erase(keyCode);
					if (modifierKeys.count(keyCode)) {
						activeModifiers.erase(keyCode);
					}

					// Check if we should clear combinations while holding the lock
					if (activeModifiers.empty() ||
					    std::none_of(activeModifiers.begin(), activeModifiers.end(),
					                 [](int key) { return pressedKeys.count(key) > 0; })) {
						loggedCombinations.clear();
					}
				}
			} else if (event.type == ButtonPress) {
				// Handle mouse button clicks (only when modifier keys are pressed)
				if (isModifierKeyPressed()) {
					std::string keyCombination = getCurrentCombination();

					// X11 button numbers: 1=Left, 2=Middle, 3=Right, 4=ScrollUp, 5=ScrollDown, 8=Back, 9=Forward
					unsigned int button = event.xbutton.button;
					switch (button) {
					case 1:
						keyCombination += " + Left Click";
						break;
					case 2:
						keyCombination += " + Middle Click";
						break;
					case 3:
						keyCombination += " + Right Click";
						break;
					case 4:
						keyCombination += " + Scroll Up";
						break;
					case 5:
						keyCombination += " + Scroll Down";
						break;
					case 6:
						keyCombination += " + Scroll Left";
						break;
					case 7:
						keyCombination += " + Scroll Right";
						break;
					case 8:
						keyCombination += " + Back Button";
						break;
					case 9:
						keyCombination += " + Forward Button";
						break;
					default:
						keyCombination += " + Button " + std::to_string(button);
						break;
					}

					blog(LOG_INFO, "[StreamUP Hotkey Display] Mouse action detected: %s",
					     keyCombination.c_str());
					if (hotkeyDisplayDock) {
						hotkeyDisplayDock->setLog(QString::fromStdString(keyCombination));
					}
				}
			}
		}
	}

	if (display) {
		XCloseDisplay(display);
		display = nullptr;
	}

	blog(LOG_INFO, "[StreamUP Hotkey Display] Linux keyboard hook thread stopped");
}

void startLinuxKeyboardHook()
{
	if (linuxHookRunning) {
		blog(LOG_WARNING, "[StreamUP Hotkey Display] Linux hook already running");
		return;
	}

	linuxHookRunning = true;
	linuxHookThread = std::thread(linuxKeyboardHookThreadFunc);
}

void stopLinuxKeyboardHook()
{
	if (!linuxHookRunning) {
		return;
	}

	blog(LOG_INFO, "[StreamUP Hotkey Display] Stopping Linux keyboard hook...");
	linuxHookRunning = false;

	// Wait for thread to finish
	if (linuxHookThread.joinable()) {
		linuxHookThread.join();
	}

	// Display is closed in the thread function
	display = nullptr;
}
#endif

void LoadHotkeyDisplayDock()
{
	const auto main_window = static_cast<QMainWindow *>(obs_frontend_get_main_window());
	obs_frontend_push_ui_translation(obs_module_get_string);

	hotkeyDisplayDock = new HotkeyDisplayDock(main_window);

	const QString title = QString::fromUtf8(obs_module_text("Dock.Title"));
	const auto name = "HotkeyDisplayDock";

	obs_frontend_add_dock_by_id(name, title.toUtf8().constData(), hotkeyDisplayDock);

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

void parseWhitelistKeys(const QString &whitelist)
{
	whitelistedKeySet.clear();

	if (whitelist.isEmpty()) {
		return;
	}

	// Split by comma and process each key
	QStringList keys = whitelist.split(',', Qt::SkipEmptyParts);
	for (const QString &key : keys) {
		QString trimmedKey = key.trimmed().toUpper();

		if (trimmedKey.isEmpty()) {
			continue;
		}

		// Map common key names to key codes
#ifdef _WIN32
		// Single character keys
		if (trimmedKey.length() == 1) {
			QChar ch = trimmedKey[0];
			if (ch.isLetter()) {
				whitelistedKeySet.insert(ch.unicode());
			} else if (ch.isDigit()) {
				whitelistedKeySet.insert(ch.unicode());
			}
		}
		// Special key names
		else if (trimmedKey == "SPACE") whitelistedKeySet.insert(VK_SPACE);
		else if (trimmedKey == "TAB") whitelistedKeySet.insert(VK_TAB);
		else if (trimmedKey == "ENTER") whitelistedKeySet.insert(VK_RETURN);
		else if (trimmedKey == "ESC" || trimmedKey == "ESCAPE") whitelistedKeySet.insert(VK_ESCAPE);
#endif

#ifdef __APPLE__
		// Single character keys
		if (trimmedKey.length() == 1) {
			QChar ch = trimmedKey[0];
			if (ch.isLetter()) {
				// Map A-Z to kVK_ANSI_A through kVK_ANSI_Z
				int offset = ch.unicode() - 'A';
				if (offset >= 0 && offset < 26) {
					whitelistedKeySet.insert(kVK_ANSI_A + offset);
				}
			} else if (ch.isDigit()) {
				// Map 0-9 to kVK_ANSI_0 through kVK_ANSI_9
				int digit = ch.digitValue();
				whitelistedKeySet.insert(kVK_ANSI_0 + digit);
			}
		}
		// Special key names
		else if (trimmedKey == "SPACE") whitelistedKeySet.insert(kVK_Space);
		else if (trimmedKey == "TAB") whitelistedKeySet.insert(kVK_Tab);
		else if (trimmedKey == "ENTER") whitelistedKeySet.insert(kVK_Return);
		else if (trimmedKey == "ESC" || trimmedKey == "ESCAPE") whitelistedKeySet.insert(kVK_Escape);
#endif

#ifdef __linux__
		// Single character keys
		if (trimmedKey.length() == 1) {
			QChar ch = trimmedKey[0];
			if (ch.isLetter()) {
				// XK_a through XK_z (lowercase)
				int lowerOffset = ch.unicode() - 'A';
				if (lowerOffset >= 0 && lowerOffset < 26) {
					whitelistedKeySet.insert(XK_a + lowerOffset);
					whitelistedKeySet.insert(XK_A + lowerOffset);
				}
			} else if (ch.isDigit()) {
				int digit = ch.digitValue();
				whitelistedKeySet.insert(XK_0 + digit);
			}
		}
		// Special key names
		else if (trimmedKey == "SPACE") whitelistedKeySet.insert(XK_space);
		else if (trimmedKey == "TAB") whitelistedKeySet.insert(XK_Tab);
		else if (trimmedKey == "ENTER") whitelistedKeySet.insert(XK_Return);
		else if (trimmedKey == "ESC" || trimmedKey == "ESCAPE") whitelistedKeySet.insert(XK_Escape);
#endif
	}
}

void loadSingleKeyCaptureSettings(obs_data_t *settings)
{
	if (!settings) {
		return;
	}

	captureNumpad = obs_data_get_bool(settings, "captureNumpad");
	captureNumbers = obs_data_get_bool(settings, "captureNumbers");
	captureLetters = obs_data_get_bool(settings, "captureLetters");
	capturePunctuation = obs_data_get_bool(settings, "capturePunctuation");

	QString whitelist = QString::fromUtf8(obs_data_get_string(settings, "whitelistedKeys"));
	parseWhitelistKeys(whitelist);
}

void loadDockSettings(HotkeyDisplayDock *dock, obs_data_t *settings)
{
	if (!dock || !settings) {
		return;
	}

	// Load settings with defaults
	dock->sceneName = QString::fromUtf8(obs_data_get_string(settings, "sceneName"));
	dock->textSource = QString::fromUtf8(obs_data_get_string(settings, "textSource"));
	dock->onScreenTime = obs_data_get_int(settings, "onScreenTime");
	dock->prefix = QString::fromUtf8(obs_data_get_string(settings, "prefix"));
	dock->suffix = QString::fromUtf8(obs_data_get_string(settings, "suffix"));
	dock->setDisplayInTextSource(obs_data_get_bool(settings, "displayInTextSource"));

	// Apply defaults if empty
	if (dock->sceneName.isEmpty()) {
		dock->sceneName = StyleConstants::DEFAULT_SCENE_NAME;
	}
	if (dock->textSource.isEmpty()) {
		dock->textSource = StyleConstants::DEFAULT_TEXT_SOURCE;
	}
	if (dock->onScreenTime == 0) {
		dock->onScreenTime = StyleConstants::DEFAULT_ONSCREEN_TIME;
	}
}

void applyDockUISettings(HotkeyDisplayDock *dock, bool hookEnabled)
{
	if (!dock) {
		return;
	}

	const char *actionText = hookEnabled ? obs_module_text("Dock.Button.Disable") : obs_module_text("Dock.Button.Enable");
	const char *state = hookEnabled ? "active" : "inactive";

	dock->getToggleAction()->setChecked(hookEnabled);
	dock->getToggleAction()->setText(actionText);
	QLabel *label = dock->getLabel();
	label->setProperty("hotkeyState", state);
	label->style()->unpolish(label);
	label->style()->polish(label);
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

	if (settings && hotkeyDisplayDock) {
		loadDockSettings(hotkeyDisplayDock, settings);
		loadSingleKeyCaptureSettings(settings);
		bool hookEnabled = obs_data_get_bool(settings, "hookEnabled");
		hotkeyDisplayDock->setHookEnabled(hookEnabled);
		applyDockUISettings(hotkeyDisplayDock, hookEnabled);
		obs_data_release(settings);
	} else if (hotkeyDisplayDock) {
		// Apply defaults if no settings loaded
		hotkeyDisplayDock->sceneName = StyleConstants::DEFAULT_SCENE_NAME;
		hotkeyDisplayDock->textSource = StyleConstants::DEFAULT_TEXT_SOURCE;
		hotkeyDisplayDock->onScreenTime = StyleConstants::DEFAULT_ONSCREEN_TIME;
		hotkeyDisplayDock->prefix = "";
		hotkeyDisplayDock->suffix = "";
		hotkeyDisplayDock->setDisplayInTextSource(false);
		applyDockUISettings(hotkeyDisplayDock, false);
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
	if (mouseHook) {
		UnhookWindowsHookEx(mouseHook);
		mouseHook = NULL;
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
	return obs_module_text("Plugin.Description");
}

MODULE_EXPORT const char *obs_module_name(void)
{
	return obs_module_text("Plugin.Name");
}
