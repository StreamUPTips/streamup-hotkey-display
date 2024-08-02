#include "streamup-hotkey-display-dock.hpp"
#include "streamup-hotkey-display-settings.hpp"
#include <windows.h>
#include <obs.h>
#include <QIcon>

// Ensure that SaveLoadSettingsCallback is declared
extern obs_data_t *SaveLoadSettingsCallback(obs_data_t *save_data, bool saving);

extern HHOOK keyboardHook;                                                     // Declare the external keyboard hook variable
extern LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam); // Declare the external keyboard procedure

HotkeyDisplayDock::HotkeyDisplayDock(QWidget *parent)
	: QFrame(parent),
	  layout(new QVBoxLayout(this)),
	  buttonLayout(new QHBoxLayout()), // Add a horizontal layout for the buttons
	  label(new QLabel(this)),
	  toggleButton(new QPushButton("Disable Hook", this)),
	  settingsButton(new QPushButton(this)),
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
}

HotkeyDisplayDock::~HotkeyDisplayDock() {}

void HotkeyDisplayDock::setLog(const QString &log)
{
	label->setText(log);
	updateTextSource(log); // Update the text source with the log
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
				     "  border: 2px solid #888888;" // Grey border for disabled state
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
					     "  border: 2px solid #4CAF50;" // Green border for enabled state
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
		textSource = settingsDialog->textSource; // Update the text source
		obs_data_release(settings);
	}

	settingsDialog->exec();
	delete settingsDialog;
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
		}
	}
}
