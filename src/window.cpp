#include "ui.h"
#include <algorithm>
#include <assert.h>

Window::Window(std::unique_ptr<Controller> &&controller):
	_controller(std::move(controller)),
	_framewin(newwin(0, 0, 0, 0)),
	_framepanel(new_panel(_framewin)),
	_contentwin(newwin(0, 0, 0, 0)),
	_contentpanel(new_panel(_contentwin))
{
	draw_chrome();
	_controller->paint(_contentwin);
}

Window::~Window()
{
	del_panel(_framepanel);
	delwin(_framewin);
	del_panel(_contentpanel);
	delwin(_contentwin);
}

void Window::layout(int xpos, int height, int width, bool lframe, bool rframe)
{
	bool needs_chrome = false;
	int frameheight = height--;
	int framewidth = width;
	int framepos = xpos;
	if (lframe != _lframe || rframe != _rframe) {
		_lframe = lframe;
		_rframe = rframe;
		needs_chrome = true;
	}
	if (_lframe) {
		framepos -= 1;
		framewidth += 1;
	}
	if (_rframe) {
		framewidth += 1;
	}
	if (frameheight != _height || framewidth != _width) {
		_height = frameheight;
		_width = framewidth;
		_xpos = framepos;
		// Resize the frame panel and create a new window with
		// the new dimensions, since you can't resize a window.
		WINDOW *replacement = newwin(_height, _width, 0, _xpos);
		replace_panel(_framepanel, replacement);
		delwin(_framewin);
		_framewin = replacement;
		needs_chrome = true;
		// Resize the content panel and give it a new window
		// as well.
		replacement = newwin(height, width, 1, xpos);
		replace_panel(_contentpanel, replacement);
		delwin(_contentwin);
		_contentwin = replacement;
	} else if (framepos != _xpos) {
		_xpos = framepos;
		move_panel(_framepanel, 0, _xpos);
		move_panel(_contentpanel, 1, xpos);
	}
	if (needs_chrome) {
		draw_chrome();
	}
	_controller->paint(_contentwin);
}

void Window::set_focus()
{
	_has_focus = true;
	draw_chrome();
}

void Window::clear_focus()
{
	_has_focus = false;
	draw_chrome();
}

void Window::bring_forward()
{
	top_panel(_framepanel);
	top_panel(_contentpanel);
}

bool Window::process(int ch, App &app)
{
	return _controller->process(_contentwin, ch, app);
}

bool Window::poll(App &app)
{
	return _controller->poll(_contentwin, app);
}

void Window::draw_chrome()
{
	// Draw the title bar.
	mvwchgat(_framewin, 0, 0, _width, A_NORMAL, 0, NULL);
	int barx = 0 + (_lframe ? 1 : 0);
	int barwidth = std::max(0, _width + (_lframe ? -1 : 0) + (_rframe ? -1 : 0));
	int titlex = barx + 1;
	int titlewidth = std::max(0, barwidth - 2);
	std::string title = _controller->title();
	if (_has_focus) {
		// Highlight the target with a big reverse-text title.
		title.resize(titlewidth, ' ');
		mvwaddch(_framewin, 0, titlex-1, ' ');
		mvwprintw(_framewin, 0, titlex, title.c_str());
		waddch(_framewin, ' ');
		mvwchgat(_framewin, 0, barx, barwidth, A_REVERSE, 0, NULL);
	} else if (title.size() >= (size_t)titlewidth) {
		// The text will completely fill the space.
		title.resize(titlewidth);
		mvwprintw(_framewin, 0, titlex, title.c_str());
	} else {
		// The text will not completely fill the bar.
		// Draw a continuing horizontal line following.
		mvwprintw(_framewin, 0, titlex, title.c_str());
		int extra = titlewidth - title.size();
		while (extra > 0) {
			mvwaddch(_framewin, 0, barx + barwidth - extra, ACS_HLINE);
			extra--;
		}
	}
	// Draw the left frame, if we have one.
	if (_lframe) {
		mvwaddch(_framewin, 0, 0, ACS_ULCORNER);
		for (int i = 1; i < _height; ++i) {
			mvwaddch(_framewin, i, 0, ACS_VLINE);
		}
	}
	// Draw the right frame, if we have one.
	if (_rframe) {
		int col = _width - 1;
		mvwaddch(_framewin, 0, col, ACS_URCORNER);
		for (int i = 1;  i < _height; ++i) {
			mvwaddch(_framewin, i, col, ACS_VLINE);
		}
	}
}

