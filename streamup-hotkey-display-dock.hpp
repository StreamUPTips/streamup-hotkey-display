#pragma once

#ifndef STREAMUP_HOTKEY_DISPLAY_DOCK_HPP
#define STREAMUP_HOTKEY_DISPLAY_DOCK_HPP

#include <QFrame>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTimer>

class HotkeyDisplayDock : public QFrame {
	Q_OBJECT

public:
	HotkeyDisplayDock(QWidget *parent = nullptr);
	~HotkeyDisplayDock();

	void setLog(const QString &log);

public slots:
	void toggleKeyboardHook();
	void openSettings();
	void clearDisplay();

private:
	QVBoxLayout *layout;
	QHBoxLayout *buttonLayout;
	QLabel *label;
	QPushButton *toggleButton;
	QPushButton *settingsButton;
	bool hookEnabled;
	QString textSource;
	QTimer *clearTimer;
	int onScreenTime;

	void updateTextSource(const QString &text);
};

#endif // STREAMUP_HOTKEY_DISPLAY_DOCK_HPP
