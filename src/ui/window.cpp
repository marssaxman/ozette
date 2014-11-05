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

void UI::Window::layout(int xpos, int width)
{
	// Windows are vertical slices of screen space.
	// Given this column number and a width, compute the
	// window's nominal dimensions, then see if we need to adjust
	// the actual window to match.
	int screen_height, screen_width;
	getmaxyx(stdscr, screen_height, screen_width);
	// We will expand our edges by one pixel in each direction if
	// we have space for it so that we can draw a window frame.
	_lframe = xpos > 0;
	if (_lframe) {
		xpos--;
		width++;
	}
	_rframe = (xpos + width) < screen_width;
	if (_rframe) {
		width++;
	}

	// What are the dimensions and location of our existing frame?
	// If the window has the wrong size, we must rebuild it, and
	// then we should layout our content window too. Otherwise, if
	// the window is in the wrong place, simply relocate it.
	int old_height, old_width;
	getmaxyx(_framewin, old_height, old_width);
	int old_vert, old_horz;
	getbegyx(_framewin, old_vert, old_horz);
	if (old_height != screen_height || old_width != width) {
		WINDOW *replacement = newwin(screen_height, width, 0, xpos);
		replace_panel(_framepanel, replacement);
		delwin(_framewin);
		_framewin = replacement;
		_dirty_chrome = true;
	} else if (old_vert != 0 || old_horz != xpos) {
		move_panel(_framepanel, 0, xpos);
	}
	layout_contentwin();
	paint();
}

void UI::Window::set_focus()
{
	_has_focus = true;
	_dirty_chrome = true;
	_dirty_content = true;
	paint();
}

void UI::Window::clear_focus()
{
	_has_focus = false;
	_dirty_chrome = true;
	_dirty_content = true;
	paint();
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

void UI::Window::set_help(const help_panel_t *help)
{
	_help = help;
	layout_taskbar();
}

void UI::Window::layout_contentwin()
{
	// Compute the location and dimension of the content window,
	// relative to the location and dimension of the frame window.
	int new_height, new_width;
	getmaxyx(_framewin, new_height, new_width);
	int new_vpos, new_hpos;
	getbegyx(_framewin, new_vpos, new_hpos);

	// Adjust these dimensions inward to account for the space
	// used by the frame. Every window has a title bar.
	new_vpos++;
	new_height--;
	// The window may have a one-column left frame.
	if (_lframe) {
		new_hpos++;
		new_width--;
	}
	// The window may have a one-column right frame.
	if (_rframe) {
		new_width--;
	}
	// There may be a task bar, whose height may vary.
	new_height -= _taskbar_height;

	// Find out how where and how large the content window is.
	// If our existing content window is the wrong size, recreate it.
	// Otherwise, if it needs to be relocated, move its panel.
	int old_height, old_width;
	getmaxyx(_contentwin, old_height, old_width);
	int old_vpos, old_hpos;
	getbegyx(_contentwin, old_vpos, old_hpos);
	if (old_height != new_height || old_width != new_width) {
		WINDOW *win = newwin(new_height, new_width, new_vpos, new_hpos);
		replace_panel(_contentpanel, win);
		delwin(_contentwin);
		_contentwin = win;
		_dirty_content = true;
	} else if (old_vpos != new_vpos || old_hpos != new_vpos) {
		move_panel(_contentpanel, new_vpos, new_hpos);
	}
}

void UI::Window::layout_taskbar()
{
	unsigned new_height = 0;
	// At present the only thing which lives on the taskbar is the help panel
	if (_help) {
		new_height += help_panel_t::kRows;
	}
	if (new_height != _taskbar_height) {
		_taskbar_height = new_height;
		_dirty_chrome = true;
		layout_contentwin();
	}
}

void UI::Window::paint()
{
	if (_dirty_content) paint_content();
	if (_dirty_chrome) paint_chrome();
}

void UI::Window::paint_content()
{
	_controller->paint(_contentwin, _has_focus);
	_dirty_content = false;
}

void UI::Window::paint_chrome()
{
	int height, width;
	getmaxyx(_framewin, height, width);
	paint_title_bar(height, width);
	if (_lframe) paint_left_frame(height, width);
	if (_rframe) paint_right_frame(height, width);
	if (_taskbar_height) paint_taskbar(height, width);
	_dirty_chrome = false;
}

void UI::Window::paint_title_bar(int height, int width)
{
	// Draw corners and a horizontal line across the top.
	mvwhline(_framewin, 0, 0, ACS_HLINE, width);
	if (_lframe) {
		mvwaddch(_framewin, 0, 0, ACS_ULCORNER);
	}
	if (_rframe) {
		mvwaddch(_framewin, 0, width-1, ACS_URCORNER);
	}

	// Overwrite the bar line with the window title.
	int left = _lframe ? 3 : 2;
	int right = width - (_rframe ? 3 : 2);
	width = right - left;
	int titlechars = width - 2;
	wmove(_framewin, 0, left);
	if (_has_focus) wattron(_framewin, A_REVERSE);
	waddch(_framewin, ' ');
	waddnstr(_framewin, _title.c_str(), titlechars);
	waddch(_framewin, ' ');
	if (_has_focus) wattroff(_framewin, A_REVERSE);

	// If there is a status string, print it on the right side.
	if (_status.empty()) return;
	int statchars = std::min((int)_status.size(), titlechars/2);
	mvwaddch(_framewin, 0, right - statchars - 2, ' ');
	waddnstr(_framewin, _status.c_str(), statchars);
	waddch(_framewin, ' ');
}

void UI::Window::paint_left_frame(int height, int width)
{
	mvwvline(_framewin, 1, 0, ACS_VLINE, height - 1);
}

void UI::Window::paint_right_frame(int height, int width)
{
	mvwvline(_framewin, 1, width-1, ACS_VLINE, height-1);
}

void UI::Window::paint_taskbar(int height, int width)
{
	int xpos = _lframe ? 1 : 0;
	width -= xpos;
	if (_rframe) width--;
	int ypos = height - _taskbar_height;
	// Clear out the space we're going to work in.
	for (int v = ypos; v < height; ++v) {
		mvwhline(_framewin, v, xpos, ' ', width);
	}

	// If there is a help panel, render it here.
	if (!_help) return;

	int labelwidth = width / help_panel_t::kColumns;
	int textwidth = labelwidth - 4;
	unsigned v = 0;
	unsigned h = 0;

	while (v < help_panel_t::kRows) {
		auto &label = _help->label[v][h];
		int labelpos = h * labelwidth;
		wmove(_framewin, v+ypos, labelpos+xpos);
		if (_has_focus) wattron(_framewin, A_REVERSE);
		waddch(_framewin, '^');
		waddch(_framewin, label.key);
		if (_has_focus) wattroff(_framewin, A_REVERSE);
		waddch(_framewin, ' ');
		waddnstr(_framewin, label.text, textwidth);
		waddch(_framewin, ' ');
		if (++h == help_panel_t::kColumns) {
			h = 0;
			v++;
		}
	}
}
