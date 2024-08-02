#include "streamup-hotkey-display-settings.hpp"

StreamupHotkeyDisplaySettings::StreamupHotkeyDisplaySettings(QWidget *parent)
	: QDialog(parent),
	  layout(new QVBoxLayout(this)),
	  closeButton(new QPushButton("Close", this))
{
	setWindowTitle("Hotkey Display Settings");
	layout->addWidget(closeButton);
	setLayout(layout);

	connect(closeButton, &QPushButton::clicked, this, &StreamupHotkeyDisplaySettings::close);
}
