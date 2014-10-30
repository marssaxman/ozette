#include "ui.h"
#include <algorithm>
#include <assert.h>

Window::Window(std::unique_ptr<Controller> &&controller, int height, int width):
	_controller(std::move(controller)),
	_window(newwin(height, width, 0, 0)),
	_panel(new_panel(_window))
{
}

Window::~Window()
{
	del_panel(_panel);
	delwin(_window);
}

void Window::move_to(int ypos, int xpos)
{
	if (ypos == _ypos && xpos == _xpos) return;
	_ypos = ypos;
	_xpos = xpos;
	move_panel(_panel, _ypos, _xpos);
}

void Window::resize(int height, int width)
{
	WINDOW *replacement = newwin(height, width, _ypos, _xpos);
	replace_panel(_panel, replacement);
	delwin(_window);
	_window = replacement;
}

void Window::set_focus()
{
	top_panel(_panel);
}

UI::UI()
{
	// Set up ncurses.
	initscr();
	cbreak();
	noecho();
	nonl();
	intrflush(stdscr, FALSE);
	keypad(stdscr, true);
	// Find out how big the terminal is.
	getmaxyx(stdscr, _height, _width);
}

UI::~UI()
{
	// Delete all of the windows.
	_columns.clear();
	// Clean up ncurses.
	endwin();
}

bool UI::process(int ch)
{
	// The UI handles control-shift-arrow-key presses by changing
	// the focus window. All other keypresses are delegated to
	// the focus window.
	switch (ch) {
		case 0x21D: {	// Control-shift-left arrow
			if (_focus > 0) {
				set_focus(_focus - 1);
			} else {
				set_focus(_columns.size() - 1);
			}
		} break;
		case 0x22C: {	// Control-shift-right arrow
			size_t next = _focus + 1;
			if (next >= _columns.size()) {
				next = 0;
			}
			set_focus(next);
		} break;
		case KEY_RESIZE: {
			getmaxyx(stdscr, _height, _width);
			int newwidth = std::min(_width, 80);
			int newheight = _height - 1;
			for (auto &win: _columns) {
				win->resize(newheight, newwidth);
			}
			relayout();
		} break;
		default: {
			_columns[_focus]->process(ch);
		} break;
	}
	// quit on tab press for now... temporary
	return ch != 9;
}

void UI::open_window(std::unique_ptr<Window::Controller> &&controller)
{
	// We reserve the top row for the title bar.
	// Aside from that, new windows fill the terminal rows.
	// Windows are never wider than 80 columns.
	int width = std::min(_width, 80);
	int height = _height - 1;
	_columns.emplace_back(new Window(std::move(controller), height, width));
	_focus = _columns.size() - 1;
	relayout();
	drawtitlebar();
}

void UI::set_focus(size_t index)
{
	assert(index >= 0 && index < _columns.size());
	if (_focus == index) return;
	_focus = index;
	_columns[_focus]->set_focus();
	drawtitlebar();
}

void UI::relayout()
{
	// The leftmost window owns column zero and covers no more than 80
	// characters' width.
	// Divide any remaining space among any remaining windows and stagger
	// each remaining window proportionally across the screen.
	_spacing = _width - 80;
	if (_columns.size() > 1) {
		_spacing /= (_columns.size() - 1);
	}
	for (unsigned i = 0; i < _columns.size(); ++i) {
		_columns[i]->move_to(1, _spacing * i);
	}
	drawtitlebar();
}

void UI::drawtitlebar()
{
	// Draw all the non-active titles using normal text.
	for (unsigned i = 0; i < _columns.size(); ++i) {
		if (i == _focus) continue;
		std::string title = preptitle(_columns[i]->title(), _spacing);
		mvprintw(0, i * _spacing, title.c_str());
	}
	// Draw the focus title bar in emphasis (reverse),
	// over the top of all other menu title bars.
	int focusx = _focus * _spacing;
	mvchgat(0, 0, -1, A_NORMAL, 0, NULL);
	std::string focustitle = preptitle(_columns[_focus]->title(), 80);
	mvprintw(0, focusx, focustitle.c_str());
	mvchgat(0, focusx, focustitle.size(), A_REVERSE, 0, NULL);
}

std::string UI::preptitle(std::string title, int barwidth)
{
	// Menu bar titles always begin and end with a space.
	// We will return a string whose length equals barwidth.
	barwidth -= 2;
	if (barwidth < 0) barwidth = 0;
	title.resize(barwidth, ' ');
	return " " + title + " ";
}

