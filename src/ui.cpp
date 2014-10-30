#include "ui.h"
#include <algorithm>
#include <assert.h>

// We allocate two columns more than requested for the window borders.
Window::Window(std::unique_ptr<Controller> &&controller, int height, int width):
	_height(height),
	_width(width),
	_controller(std::move(controller)),
	_window(newwin(_height, _width, 0, 0)),
	_panel(new_panel(_window))
{
	draw_chrome();
}

Window::~Window()
{
	del_panel(_panel);
	delwin(_window);
}

void Window::layout(int xpos, int height, int width)
{
	if (height != _height || width != _width) {
		_height = height;
		_width = width;
		_xpos = xpos;
		WINDOW *replacement = newwin(_height, _width, 0, _xpos);
		replace_panel(_panel, replacement);
		delwin(_window);
		_window = replacement;
		draw_chrome();
	} else if (xpos != _xpos) {
		_xpos = xpos;
		move_panel(_panel, 0, _xpos);
	}
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

void Window::draw_chrome()
{
	// Draw the title bar.
	mvwprintw(_window, 0, 1, _controller->title().c_str());
	clrtoeol();
        mvwchgat(_window, 0, 0, _width, _has_focus ? A_REVERSE : A_NORMAL, 0, NULL);
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
	get_screen_size();
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
			get_screen_size();
			relayout();
		} break;
		default: {
			_columns[_focus]->process(ch);
		} break;
	}
	// quit on tab press for now... temporary
	return ch != 9;
}

void UI::get_screen_size()
{
	getmaxyx(stdscr, _height, _width);
	_columnWidth = std::min(80, _width);
}

void UI::open_window(std::unique_ptr<Window::Controller> &&controller)
{
	// We reserve the top row for the title bar.
	// Aside from that, new windows fill the terminal rows.
	// Windows are never wider than 80 columns.
	_columns.emplace_back(new Window(std::move(controller), _height, _columnWidth));
	relayout();
	set_focus(_columns.size() - 1);
}

void UI::set_focus(size_t index)
{
	assert(index >= 0 && index < _columns.size());
	if (_focus == index) return;
	_columns[_focus]->clear_focus();
	_focus = index;
	_columns[_focus]->set_focus();
}

int UI::column_left(size_t index)
{
	// What is the best X coordinate to use for this column?
	// It is possible that the number of columns does not divide
	// evenly into the screen space available. We will pin the
	// zero column to the left, then distribute other columns
	// from the right, to create a pleasing appearance.
	if (index == 0) return 0;
	size_t ubound = _columns.size() - 1;
	return _width - _columnWidth - (ubound - index) * _spacing;
}

void UI::relayout()
{
	// The leftmost window owns column zero and covers no more than 80
	// characters' width.
	// Divide any remaining space among any remaining windows and stagger
	// each remaining window proportionally across the screen.
	_spacing = _width - _columnWidth;
	if (_columns.size() > 1) {
		_spacing /= (_columns.size() - 1);
	}
	for (unsigned i = 0; i < _columns.size(); ++i) {
		_columns[i]->layout(column_left(i), _height, _columnWidth);
	}
}

