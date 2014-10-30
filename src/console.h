#ifndef CONSOLE_H
#define CONSOLE_H

#include "ui.h"

class Console : public Window::Controller
{
public:
	virtual bool process(Window &window, int ch) override;
	virtual std::string title() const override { return "Console"; }
};

#endif // CONSOLE_H
