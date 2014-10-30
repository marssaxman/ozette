#ifndef BROWSER_H
#define BROWSER_H

#include "ui.h"

class Browser : public Window::Controller
{
public:
	virtual bool process(Window &window, int ch) override;
	virtual std::string title() const override { return "Browser"; }
};

#endif // BROWSER_H
