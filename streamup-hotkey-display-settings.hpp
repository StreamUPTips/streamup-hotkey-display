#pragma once

#ifndef STREAMUP_HOTKEY_DISPLAY_SETTINGS_HPP
#define STREAMUP_HOTKEY_DISPLAY_SETTINGS_HPP

#include <QDialog>
#include <QVBoxLayout>
#include <QPushButton>

class StreamupHotkeyDisplaySettings : public QDialog {
	Q_OBJECT

public:
	StreamupHotkeyDisplaySettings(QWidget *parent = nullptr);

private:
	QVBoxLayout *layout;
	QPushButton *closeButton;
};

#endif // STREAMUP_HOTKEY_DISPLAY_SETTINGS_HPP
