#pragma once

#ifndef STREAMUP_HOTKEY_DISPLAY_DOCK_HPP
#define STREAMUP_HOTKEY_DISPLAY_DOCK_HPP

#include <QFrame>
#include <QVBoxLayout>
#include <QLabel>

class HotkeyDisplayDock : public QFrame {
	Q_OBJECT

public:
	HotkeyDisplayDock(QWidget *parent = nullptr);
	~HotkeyDisplayDock();

	void setLog(const QString &log);

private:
	QVBoxLayout *layout;
	QLabel *label;
};

#endif // STREAMUP_HOTKEY_DISPLAY_DOCK_HPP
