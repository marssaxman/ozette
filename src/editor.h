#ifndef EDITOR_H
#define EDITOR_H

#include "controller.h"

class Editor : public Controller
{
public:
	Editor(std::string targetpath);
	virtual void paint(View &view) override;
	virtual bool process(View &view, int ch) override;
	virtual std::string title() const override { return _targetpath; }
private:
	std::string _targetpath;
	std::vector<std::string> _lines;
	size_t _scrollpos = 0;
};

#endif // CONSOLE_H
