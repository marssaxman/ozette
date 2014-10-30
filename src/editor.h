#ifndef EDITOR_H
#define EDITOR_H

#include "ui.h"

class Editor : public Window::Controller
{
public:
	Editor(std::string target);
	virtual bool process(Window &window, int ch) override;
	virtual std::string title() const override { return _target; }
private:
	std::string _target;
};

#endif // CONSOLE_H
