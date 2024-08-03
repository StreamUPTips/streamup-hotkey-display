#pragma once

#ifndef STREAMUP_HOTKEY_DISPLAY_DOCK_HPP
#define STREAMUP_HOTKEY_DISPLAY_DOCK_HPP

#include <QFrame>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <obs.h>

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

public: // Make these variables public
	QString sceneName;
	QString textSource;
	int onScreenTime;

private:
	QVBoxLayout *layout;
	QHBoxLayout *buttonLayout;
	QLabel *label;
	QPushButton *toggleButton;
	QPushButton *settingsButton;
	bool hookEnabled;
	QTimer *clearTimer;

	void updateTextSource(const QString &text);
	void showSource();
	void hideSource();
};

#endif // STREAMUP_HOTKEY_DISPLAY_DOCK_HPP
