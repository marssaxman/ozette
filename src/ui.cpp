#include "ui.h"
#include <algorithm>
#include <assert.h>
#include <list>

UI::UI(Delegate &host):
	_host(host)
{
	// Set up ncurses.
	initscr();
	cbreak();
	noecho();
	nonl();
	raw();
	intrflush(stdscr, FALSE);
	keypad(stdscr, true);
	// Find out how big the terminal is.
	get_screen_size();
	// temporary: there will have to be some logic
	// for showing the cursor in editor windows
	curs_set(0);
}

UI::~UI()
{
	// Delete all of the windows.
	_columns.clear();
	// Clean up ncurses.
	endwin();
}

bool UI::process(int ch, App &app)
{
	// The UI handles control-shift-arrow-key presses by changing
	// the focus window. All other keypresses are delegated to
	// the focus window.
	switch (ch) {
		case ERR: {	// timeout
			std::list<size_t> dead;
			for (size_t i = 0; i < _columns.size(); ++i) {
				auto &win = _columns[i];
				if (!win->poll(app)) {
					dead.push_front(i);
				}
			}
			for (auto i: dead) {
				close_window(i);
			}
			relayout();
		} break;
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
			send_to_focus(ch, app);
		} break;
	}
	return !_columns.empty();
}

void UI::get_screen_size()
{
	getmaxyx(stdscr, _height, _width);
	_columnWidth = std::min(80, _width);
}

Window *UI::open_window(std::unique_ptr<Controller> &&controller)
{
	// We reserve the top row for the title bar.
	// Aside from that, new windows fill the terminal rows.
	// Windows are never wider than 80 columns.
	Window *win = new Window(std::move(controller));
	_columns.emplace_back(win);
	relayout();
	set_focus(_columns.size() - 1);
	return win;
}

void UI::make_active(Window *window)
{
	for (unsigned i = 0; i < _columns.size(); ++i) {
		if (_columns[i].get() == window) {
			set_focus(i);
		}
	}
}

void UI::set_focus(size_t index)
{
	assert(index >= 0 && index < _columns.size());
	if (_focus == index) return;
	_columns[_focus]->clear_focus();
	_focus = index;
	_columns[_focus]->set_focus();
	// We want to keep as much of the background
	// visible as we can. This means we must stack
	// windows on the left of the focus in ascending
	// order, while windows to the right of the focus
	// are stacked in descending order. We raise the
	// focus window last.
	for (size_t i = 0; i < _focus; ++i) {
		_columns[i]->bring_forward();
	}
	for (size_t i = _columns.size() - 1; i > _focus; --i) {
		_columns[i]->bring_forward();
	}
	_columns[_focus]->bring_forward();
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
		int offset = (ubound - i) * _spacing;
		int xpos = (i > 0) ? right_edge - offset : 0;
		int rcoord = xpos + _columnWidth;
		bool rframe = rcoord < _width;
		_columns[i]->layout(xpos, _height, _columnWidth, lframe, rframe);
	}
}

void UI::send_to_focus(int ch, App &app)
{
	bool more = _columns[_focus]->process(ch, app);
	if (more) return;
	close_window(_focus);
	relayout();
}

void UI::close_window(size_t index)
{
	_host.window_closed(std::move(_columns.at(index)));
	_columns.erase(_columns.begin() + index);
}
