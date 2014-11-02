#include "ui.h"
#include <algorithm>
#include <assert.h>

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

void UI::open_window(std::unique_ptr<Controller> &&controller)
{
	// We reserve the top row for the title bar.
	// Aside from that, new windows fill the terminal rows.
	// Windows are never wider than 80 columns.
	_columns.emplace_back(new Window(std::move(controller)));
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

void UI::relayout()
{
	// The leftmost window owns column zero and covers no more than 80
	// characters' width.
	// Divide any remaining space among any remaining windows and stagger
	// each remaining window proportionally across the screen.
	assert(_width >= _columnWidth);
	assert(!_columns.empty());
	size_t ubound = _columns.size() - 1;
	int right_edge = _width - _columnWidth;
	_spacing = (ubound > 0) ? right_edge / ubound : 0;
	bool is_cramped = 0 == right_edge || 0 == _spacing;
	for (unsigned i = 0; i <= ubound; ++i) {
		bool lframe = i > 0 && !is_cramped;
		bool rframe = i < ubound && !is_cramped;
		int offset = (ubound - i) * _spacing;
		int xpos = (i > 0) ? right_edge - offset : 0;
		_columns[i]->layout(xpos, _height, _columnWidth, lframe, rframe);
	}
}

