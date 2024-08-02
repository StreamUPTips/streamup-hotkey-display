#include "streamup-hotkey-display-dock.hpp"
#include <windows.h>
#include <obs.h>

extern HHOOK keyboardHook;                                                     // Declare the external keyboard hook variable
extern LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam); // Declare the external keyboard procedure

HotkeyDisplayDock::HotkeyDisplayDock(QWidget *parent)
	: QFrame(parent),
	  layout(new QVBoxLayout(this)),
	  label(new QLabel(this)),
	  toggleButton(new QPushButton("Disable Hook", this)),
	  hookEnabled(true)
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

	// Add the toggle button
	toggleButton->setFixedHeight(30);
	layout->addWidget(toggleButton);
	setLayout(layout);

	// Connect the button click to the slot
	connect(toggleButton, &QPushButton::clicked, this, &HotkeyDisplayDock::toggleKeyboardHook);
}

HotkeyDisplayDock::~HotkeyDisplayDock() {}

void HotkeyDisplayDock::setLog(const QString &log)
{
	label->setText(log);
}

void HotkeyDisplayDock::toggleKeyboardHook()
{
	if (hookEnabled) {
		if (keyboardHook) {
			UnhookWindowsHookEx(keyboardHook);
			keyboardHook = NULL;
		}
		toggleButton->setText("Enable Hook");
	} else {
		keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, NULL, 0);
		if (!keyboardHook) {
			blog(LOG_ERROR, "[StreamUP Hotkey Display] Failed to set keyboard hook!");
		} else {
			toggleButton->setText("Disable Hook");
		}
	}
	hookEnabled = !hookEnabled;
}
