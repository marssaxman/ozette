#ifndef UI_WINDOW_H
#define UI_WINDOW_H

#include <ncurses.h>
#include <panel.h>
#include <memory>
#include <vector>
#include "controller.h"

class Window
{
public:
	Window(std::unique_ptr<Controller> &&controller);
	~Window();
	void layout(int xpos, int height, int width, bool lframe, bool rframe);
	void set_focus();
	void clear_focus();
	void bring_forward();
	bool process(int ch, App &app);
	bool poll(App &app);
protected:
	void draw_chrome();
private:
	int _xpos = 0;
	int _height = 0;
	int _width = 0;
	std::unique_ptr<Controller> _controller;
	WINDOW *_framewin = nullptr;
	PANEL *_framepanel = nullptr;
	WINDOW *_contentwin = nullptr;
	PANEL *_contentpanel = nullptr;
	bool _has_focus = true;
	bool _lframe = false;
	bool _rframe = false;
};

#endif // UI_WINDOW_H
