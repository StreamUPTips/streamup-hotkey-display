#pragma once

#ifndef STREAMUP_HOTKEY_DISPLAY_DOCK_HPP
#define STREAMUP_HOTKEY_DISPLAY_DOCK_HPP

#include <QFrame>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>

class HotkeyDisplayDock : public QFrame {
	Q_OBJECT

public:
	HotkeyDisplayDock(QWidget *parent = nullptr);
	~HotkeyDisplayDock();

	void setLog(const QString &log);

public slots:
	void toggleKeyboardHook();
	void openSettings();

private:
	QVBoxLayout *layout;
	QHBoxLayout *buttonLayout;
	QLabel *label;
	QPushButton *toggleButton;
	QPushButton *settingsButton;
	bool hookEnabled;
	QString textSource; // Add this line to store the selected text source

	void updateTextSource(const QString &text); // Add this line for the method to update text source
};

#endif // STREAMUP_HOTKEY_DISPLAY_DOCK_HPP
