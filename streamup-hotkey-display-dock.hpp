#pragma once

#ifndef STREAMUP_HOTKEY_DISPLAY_DOCK_HPP
#define STREAMUP_HOTKEY_DISPLAY_DOCK_HPP

#include <QFrame>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QAction>
#include <QToolBar>
#include <QTimer>
#include <obs.h>

// Default value constants
namespace StyleConstants {
constexpr const char *DEFAULT_SCENE_NAME = "Default Scene";
constexpr const char *DEFAULT_TEXT_SOURCE = "Default Text Source";
constexpr const char *NO_TEXT_SOURCE = "No text source available";
constexpr int DEFAULT_ONSCREEN_TIME = 100;
} // namespace StyleConstants

class HotkeyDisplayDock : public QFrame {
	Q_OBJECT

public:
	HotkeyDisplayDock(QWidget *parent = nullptr);
	~HotkeyDisplayDock();

	void setLog(const QString &log);
	void setDisplayInTextSource(bool enabled) { displayInTextSource = enabled; }

public slots:
	void toggleKeyboardHook();
	void openSettings();
	void clearDisplay();

	bool isHookEnabled() const { return hookEnabled; }
	void setHookEnabled(bool enabled) { hookEnabled = enabled; }
	QAction *getToggleAction() const { return toggleAction; }
	QLabel *getLabel() const { return label; }

public:
	QVBoxLayout *layout;
	QToolBar *toolbar;
	QLabel *label;
	QAction *toggleAction;
	QAction *settingsAction;
	bool hookEnabled;
	QString sceneName;
	QString textSource;
	int onScreenTime;
	QString prefix;
	QString suffix;
	QTimer *clearTimer;
	bool displayInTextSource;

private:
	void updateTextSource(const QString &text);
	void showSource();
	void hideSource();
	bool sceneAndSourceExist();
	void stopAllActivities();
	void resetToListeningState();

	// Hook management helpers
	bool enableHooks();
	void disableHooks();
	void updateUIState(bool enabled);
};

#endif // STREAMUP_HOTKEY_DISPLAY_DOCK_HPP
