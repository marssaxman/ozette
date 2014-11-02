#ifndef EDITOR_H
#define EDITOR_H

#include "controller.h"
#include <vector>

class Editor : public Controller
{
public:
	Editor(std::string targetpath);
	virtual void paint(WINDOW *view) override;
	virtual bool process(WINDOW *view, int ch) override;
	virtual std::string title() const override { return _targetpath; }
protected:
	size_t maxscroll(WINDOW *view);
private:
	std::string _targetpath;
	std::vector<std::string> _lines;
	size_t _scrollpos = 0;
};

#endif // CONSOLE_H
