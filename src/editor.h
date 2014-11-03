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
	virtual bool poll(WINDOW *view) override { return true; }
	virtual std::string title() const override { return _targetpath; }
protected:
	void arrow_up(WINDOW *view);
	void arrow_down(WINDOW *view);
	void page_up(WINDOW *view);
	void page_down(WINDOW *view);
	size_t maxscroll(WINDOW *view);
private:
	std::string _targetpath;
	std::vector<std::string> _lines;
	size_t _scrollpos = 0;
};

#endif // CONSOLE_H
