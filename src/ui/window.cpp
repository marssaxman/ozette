#include "shell.h"
#include "control.h"
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
	if (_dialog) _dialog->set_focus();
	paint();
}

void UI::Window::clear_focus()
{
	_has_focus = false;
	_dirty_chrome = true;
	_dirty_content = true;
	if (_dialog) _dialog->clear_focus();
	paint();
}

void UI::Window::bring_forward(int focus_relative)
{
	top_panel(_framepanel);
	top_panel(_contentpanel);
	if (_dialog) _dialog->bring_forward();
	bool swap_titlebar = focus_relative > 0;
	if (swap_titlebar != _swap_titlebar) {
		_swap_titlebar = swap_titlebar;
		_dirty_chrome = true;
		paint();
	}
}

bool UI::Window::process(int ch)
{
	if (_dialog) {
		if (_dialog->process(*this, ch)) return true;
		_dialog.reset();
		paint();
		return true;
	}
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

void UI::Window::set_help(const Control::Panel &help)
{
	_help = help;
	layout_taskbar();
}

void UI::Window::show_dialog(std::unique_ptr<Dialog> &&host)
{
	_dialog = std::move(host);
	_dialog->layout(_contentwin);
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

	// If we have a dialog box open, tell it how to lay itself out.
	if (_dialog) {
		_dialog->layout(_contentwin);
	}
}

void UI::Window::layout_taskbar()
{
	unsigned new_height = 0;
	// The help panel occupies two lines
	new_height += Control::Panel::height;
	// Is that the height we planned for?
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
	paint_titlebar(width);
	if (_lframe) paint_left_frame(height, width);
	if (_rframe) paint_right_frame(height, width);
	if (_taskbar_height) paint_taskbar(height, width);
	_dirty_chrome = false;
}

void UI::Window::paint_titlebar(int width)
{
	// Draw corners and a horizontal line across the top.
	mvwhline(_framewin, 0, 0, ACS_HLINE, width);
	if (_lframe) {
		mvwaddch(_framewin, 0, 0, ACS_ULCORNER);
	}
	if (_rframe) {
		mvwaddch(_framewin, 0, width-1, ACS_URCORNER);
	}
	paint_titlebar_left(width, _swap_titlebar? _status: _title);
	paint_titlebar_right(width, _swap_titlebar? _title: _status);
}

void UI::Window::paint_titlebar_left(int width, std::string text)
{
	if (text.empty()) return;
	int left = _lframe ? 2 : 1;
	int right = width - (_rframe ? 2 : 1);
	width = right - left;
	int titlechars = width - 2;
	if (_swap_titlebar) titlechars /= 2;
	wmove(_framewin, 0, left);
	if (_has_focus) wattron(_framewin, A_REVERSE);
	waddch(_framewin, ' ');
	waddnstr(_framewin, text.c_str(), titlechars);
	waddch(_framewin, ' ');
	if (_has_focus) wattroff(_framewin, A_REVERSE);
}

void UI::Window::paint_titlebar_right(int width, std::string text)
{
	if (text.empty()) return;
	int left = _lframe ? 2 : 1;
	int right = width - (_rframe ? 2 : 1);
	width = right - left;
	int titlechars = width - 2;
	if (!_swap_titlebar) titlechars /= 2;
	int chars = std::min((int)text.size(), titlechars);
	mvwaddch(_framewin, 0, right - chars - 2, ' ');
	waddnstr(_framewin, text.c_str(), chars);
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

	// Render the help panel for this window.
	int labelwidth = width / Control::Panel::width;
	int textwidth = labelwidth - 4;
	unsigned v = 0;
	unsigned h = 0;

	int key_highlight = _has_focus ? A_REVERSE : 0;
	int cells = Control::Panel::width * Control::Panel::height;
	for (int i = 0; i < cells; ++i) {
		v = i / Control::Panel::width;
		h = i % Control::Panel::width;
		unsigned ctl = _help.label[v][h];
		unsigned labelpos = h * labelwidth;
		wmove(_framewin, v+ypos, labelpos+xpos);
		if (!ctl) {
			waddnstr(_framewin, " -", textwidth);
			continue;
		}
		wattron(_framewin, key_highlight);
		waddch(_framewin, '^');
		waddch(_framewin, Control::keys[ctl].mnemonic);
		wattroff(_framewin, key_highlight);
		waddch(_framewin, ' ');
		waddnstr(_framewin, Control::keys[ctl].label, textwidth);
		waddch(_framewin, ' ');
	}
}
