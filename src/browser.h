#ifndef BROWSER_H
#define BROWSER_H

#include "controller.h"

class Browser : public Controller
{
public:
	virtual void paint(View &view) override;
	virtual bool process(View &view, int ch) override;
	virtual std::string title() const override { return "Browser"; }
};

#endif // BROWSER_H
