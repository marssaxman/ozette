#include "shell.h"
#include <algorithm>
#include <assert.h>

UI::Window::Window(App &app, std::unique_ptr<Controller> &&controller):
	_app(app),
	_controller(std::move(controller)),
	_framewin(newwin(0, 0, 0, 0)),
	_framepanel(new_panel(_framewin)),
	_contentwin(newwin(0, 0, 0, 0)),
	_contentpanel(new_panel(_contentwin))
{
	_controller->open(*this);
	paint();
}

UI::Window::~Window()
{
	del_panel(_framepanel);
	delwin(_framewin);
	del_panel(_contentpanel);
	delwin(_contentwin);
}

void UI::Window::layout(int xpos, int height, int width, bool lframe, bool rframe)
{
	int frameheight = height--;
	int framewidth = width;
	int framepos = xpos;
	if (lframe != _lframe || rframe != _rframe) {
		_lframe = lframe;
		_rframe = rframe;
		_dirty_chrome = true;
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
		_dirty_chrome = true;
		// Resize the content panel and give it a new window
		// as well.
		replacement = newwin(height, width, 1, xpos);
		replace_panel(_contentpanel, replacement);
		delwin(_contentwin);
		_contentwin = replacement;
		_dirty_content = true;
	} else if (framepos != _xpos) {
		_xpos = framepos;
		move_panel(_framepanel, 0, _xpos);
		move_panel(_contentpanel, 1, xpos);
	}
	paint();
}

void UI::Window::set_focus()
{
	_has_focus = true;
	_dirty_chrome = true;
	_dirty_content = true;
}

void UI::Window::clear_focus()
{
	_has_focus = false;
	_dirty_chrome = true;
	_dirty_content = true;
}

void UI::Window::bring_forward()
{
	top_panel(_framepanel);
	top_panel(_contentpanel);
}

bool UI::Window::process(int ch)
{
	bool out = _controller->process(*this, ch);
	paint();
	return out;
}

bool UI::Window::poll()
{
	return process(ERR);
}

void UI::Window::set_title(std::string text)
{
	_title = text;
	_dirty_chrome = true;
}

void UI::Window::set_status(std::string text)
{
	_status = text;
	_dirty_chrome = true;
}

void UI::Window::paint()
{
	if (_dirty_content) {
		_controller->paint(_contentwin, _has_focus);
		_dirty_content = false;
	}
	if (!_dirty_chrome) return;
	// Draw the left frame, if we have one.
	int barx = 0;
	int barwidth = _width;
	if (_lframe) {
		barx = 1;
		barwidth--;
		mvwaddch(_framewin, 0, 0, ACS_ULCORNER);
		for (int i = 1; i < _height; ++i) {
			mvwaddch(_framewin, i, 0, ACS_VLINE);
		}
	}
	// Draw the right frame, if we have one.
	if (_rframe) {
		barwidth--;
		int col = _width - 1;
		mvwaddch(_framewin, 0, col, ACS_URCORNER);
		for (int i = 1;  i < _height; ++i) {
			mvwaddch(_framewin, i, col, ACS_VLINE);
		}
	}
	// Draw the bar across the top of the window.
	wmove(_framewin, 0, barx);
	for (int i = 0; i < barwidth; ++i) {
		waddch(_framewin, ACS_HLINE);
	}
	// Print the window title, erasing part of the top border.
	if (barwidth > 0) {
		mvwaddch(_framewin, 0, ++barx, ' ');
	}
	waddnstr(_framewin, _title.c_str(), std::max(0, barwidth-3));
	barwidth -= _title.size();
	if (barwidth > 0) {
		waddch(_framewin, ' ');
	}
	// Draw the status text on the right side of the top line.
	if (!_status.empty()) {
		int hpos = _width - _status.size();
		mvwaddnstr(_framewin, 0, hpos, _status.c_str(), _status.size());
	}
	_dirty_chrome = false;
}

