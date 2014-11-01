#include "ui.h"
#include <algorithm>
#include <assert.h>

Window::Window(std::unique_ptr<Controller> &&controller, int height, int width):
	_height(height),
	_width(width),
	_controller(std::move(controller)),
	_window(newwin(_height, _width, 0, 0)),
	_panel(new_panel(_window))
{
	draw_chrome();
	_controller->paint(_contents);
}

Window::~Window()
{
	del_panel(_panel);
	delwin(_window);
}

void Window::layout(int xpos, int height, int width, bool lframe, bool rframe)
{
	bool needs_chrome = false;
	int bufferwidth = width;
	if (lframe != _lframe || rframe != _rframe) {
		_lframe = lframe;
		_rframe = rframe;
		needs_chrome = true;
	}
	if (_lframe) {
		xpos -= 1;
		width += 1;
	}
	if (_rframe) {
		width += 1;
	}
	if (height != _height || width != _width) {
		_height = height;
		_width = width;
		_xpos = xpos;
		WINDOW *replacement = newwin(_height, _width, 0, _xpos);
		replace_panel(_panel, replacement);
		delwin(_window);
		_window = replacement;
		needs_chrome = true;
	} else if (xpos != _xpos) {
		_xpos = xpos;
		move_panel(_panel, 0, _xpos);
	}
	if (needs_chrome) {
		draw_chrome();
	}
	_contents.layout(_window, 1, _lframe ? 1 : 0, height - 1, bufferwidth);
	_controller->paint(_contents);
}

void Window::set_focus()
{
	_has_focus = true;
	top_panel(_panel);
	draw_chrome();
}

void Window::clear_focus()
{
	_has_focus = false;
	draw_chrome();
}

bool Window::process(int ch)
{
	return _controller->process(_contents, ch);
}

void Window::draw_chrome()
{
	// Draw the title bar.
	mvwchgat(_window, 0, 0, _width, A_NORMAL, 0, NULL);
	int barx = 0 + (_lframe ? 1 : 0);
	int barwidth = std::max(0, _width + (_lframe ? -1 : 0) + (_rframe ? -1 : 0));
	int titlex = barx + 1;
	int titlewidth = std::max(0, barwidth - 2);
	std::string title = _controller->title();
	if (_has_focus) {
		// Highlight the target with a big reverse-text title.
		title.resize(titlewidth, ' ');
		mvwaddch(_window, 0, titlex-1, ' ');
		mvwprintw(_window, 0, titlex, title.c_str());
		waddch(_window, ' ');
		mvwchgat(_window, 0, barx, barwidth, A_REVERSE, 0, NULL);
	} else if (title.size() >= (size_t)titlewidth) {
		// The text will completely fill the space.
		title.resize(titlewidth);
		mvwprintw(_window, 0, titlex, title.c_str());
	} else {
		// The text will not completely fill the bar.
		// Draw a continuing horizontal line following.
		mvwprintw(_window, 0, titlex, title.c_str());
		int extra = titlewidth - title.size();
		while (extra > 0) {
			mvwaddch(_window, 0, barx + barwidth - extra, ACS_HLINE);
			extra--;
		}
	}
	// Draw the left frame, if we have one.
	if (_lframe) {
		mvwaddch(_window, 0, 0, ACS_ULCORNER);
		for (int i = 1; i < _height; ++i) {
			mvwaddch(_window, i, 0, ACS_VLINE);
		}
	}
	// Draw the right frame, if we have one.
	if (_rframe) {
		int col = _width - 1;
		mvwaddch(_window, 0, col, ACS_URCORNER);
		for (int i = 1;  i < _height; ++i) {
			mvwaddch(_window, i, col, ACS_VLINE);
		}
	}
}

