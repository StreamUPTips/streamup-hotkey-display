#pragma once

#ifndef STREAMUP_HOTKEY_DISPLAY_SETTINGS_HPP
#define STREAMUP_HOTKEY_DISPLAY_SETTINGS_HPP

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QComboBox>
#include <QSpinBox>
#include <QLabel>
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
	QVBoxLayout *mainLayout;
	QHBoxLayout *buttonLayout;
	QHBoxLayout *sourceLayout;
	QHBoxLayout *timeLayout;
	QLabel *sourceLabel;
	QLabel *timeLabel;
	QComboBox *sourceComboBox;
	QSpinBox *timeSpinBox;
	QPushButton *applyButton;
	QPushButton *closeButton;

private slots:
	void applySettings();
};

#endif // STREAMUP_HOTKEY_DISPLAY_SETTINGS_HPP
