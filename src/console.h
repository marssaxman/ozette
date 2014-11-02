#ifndef CONSOLE_H
#define CONSOLE_H

#include "controller.h"

class Console : public Controller
{
public:
	virtual void paint(WINDOW *view) override;
	virtual bool process(WINDOW *view, int ch) override;
	virtual std::string title() const override { return "Console"; }
};

#endif // CONSOLE_H
