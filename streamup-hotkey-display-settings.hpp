#pragma once

#ifndef STREAMUP_HOTKEY_DISPLAY_SETTINGS_HPP
#define STREAMUP_HOTKEY_DISPLAY_SETTINGS_HPP

#include <QDialog>
#include <QVBoxLayout>
#include <QPushButton>
#include <obs-frontend-api.h>

class StreamupHotkeyDisplaySettings : public QDialog {
	Q_OBJECT

public:
	StreamupHotkeyDisplaySettings(QWidget *parent = nullptr);

	void LoadSettings(obs_data_t *settings);
	void SaveSettings();

	QString textSource;
	int onScreenTime;

private:
	QVBoxLayout *layout;
	QPushButton *closeButton;
};

#endif // STREAMUP_HOTKEY_DISPLAY_SETTINGS_HPP
