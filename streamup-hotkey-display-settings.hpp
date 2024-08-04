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
#include <QLineEdit>
#include <obs-frontend-api.h>
#include <streamup-hotkey-display-dock.hpp>

class StreamupHotkeyDisplaySettings : public QDialog {
	Q_OBJECT

public:
	StreamupHotkeyDisplaySettings(HotkeyDisplayDock *dock, QWidget *parent);
	void LoadSettings(obs_data_t *settings);
	void SaveSettings();

	void PopulateSceneComboBox();
	void PopulateSourceComboBox(const QString &sceneName);

	QString sceneName;
	QString textSource;
	int onScreenTime;

private:
	HotkeyDisplayDock *hotkeyDisplayDock;
	QVBoxLayout *mainLayout;
	QHBoxLayout *buttonLayout;
	QHBoxLayout *sceneLayout;
	QHBoxLayout *sourceLayout;
	QHBoxLayout *timeLayout;
	QHBoxLayout *prefixLayout;
	QHBoxLayout *suffixLayout;
	QLabel *prefixLabel;
	QLabel *suffixLabel;
	QLineEdit *prefixLineEdit;
	QLineEdit *suffixLineEdit;
	QLabel *sceneLabel;
	QLabel *sourceLabel;
	QLabel *timeLabel;
	QComboBox *sceneComboBox;
	QComboBox *sourceComboBox;
	QSpinBox *timeSpinBox;
	QPushButton *applyButton;
	QPushButton *closeButton;

private slots:
	void applySettings();
	void onSceneChanged(const QString &sceneName);
};

#endif // STREAMUP_HOTKEY_DISPLAY_SETTINGS_HPP
