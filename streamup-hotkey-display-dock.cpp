#include "streamup-hotkey-display-dock.hpp"

HotkeyDisplayDock::HotkeyDisplayDock(QWidget *parent) : QFrame(parent), layout(new QVBoxLayout(this)), label(new QLabel(this))
{
	label->setAlignment(Qt::AlignCenter);
	label->setStyleSheet("QLabel {"
			     "  border: 2px solid #4CAF50;"
			     "  padding: 10px;"
			     "  border-radius: 10px;"
			     "  font-size: 18px;"
			     "  color: #FFFFFF;"
			     "  background-color: #333333;"
			     "}");
	label->setFixedHeight(50);
	layout->addWidget(label);
	setLayout(layout);
}

HotkeyDisplayDock::~HotkeyDisplayDock() {}

void HotkeyDisplayDock::setLog(const QString &log)
{
	label->setText(log);
}
