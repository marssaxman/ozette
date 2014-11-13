//
// lindi
// Copyright (C) 2014 Mars J. Saxman
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//

#include "shell.h"
#include <algorithm>
#include <assert.h>
#include <list>

UI::Shell::Shell(App &app):
	_app(app)
{
	// Set up ncurses.
	initscr();
	// Don't automatically echo characters back to the
	// screen; we will draw things ourselves when we want
	// them to appear.
	noecho();
	// We definitely want to be able to detect the return
	// key, so disable linefeed detection.
	nonl();
	// Don't let the terminal eat control keys; we want
	// to process them ourselves, in part because we will
	// be passing them along to subprocesses, and in part
	// because I have unconventional plans for some of the
	// traditional terminal control key combinations.
	raw();
	// If we print a character at the end of the bottom
	// line, don't scroll the screen up - leave everything
	// alone. We will implement our own scrolling.
	scrollok(stdscr, FALSE);
	// don't flush the output buffer when the user presses
	// one of the traditional interrupt keys, since we have
	// different purposes for them.
	intrflush(stdscr, FALSE);
	// Enable the function keys, so the user can navigate
	// using the arrow keys.
	keypad(stdscr, true);
	// By default, don't show the cursor; editors will
	// reveal it when active if they choose.
	curs_set(0);
	// Reduce the escape delay, since nobody is going to
	// use this on an old serial line, and it's much more
	// useful to be able to cancel things with the escape
	// key than to use it to type control characters.
	set_escdelay(25);
	// Find out how big the terminal is.
	get_screen_size();
}

UI::Shell::~Shell()
{
	// Delete all of the windows.
	_tabs.clear();
	// Clean up ncurses.
	endwin();
}

bool UI::Shell::process(int ch)
{
	// The UI handles control-shift-arrow-key presses by changing
	// the focus window. All other keypresses are delegated to
	// the focus window.
	switch (ch) {
		case Control::LeftArrow: {
			if (_focus > 0) {
				set_focus(_focus - 1);
			} else {
				set_focus(_tabs.size() - 1);
			}
		} break;
		case Control::RightArrow: {
			size_t next = _focus + 1;
			if (next >= _tabs.size()) {
				next = 0;
			}
			set_focus(next);
		} break;
		case Control::Quit: {
			for (size_t index = _tabs.size(); index > 0; --index) {
				if (!_tabs[index - 1]->process(Control::Close)) {
					close_window(index - 1);
				}
			}
		} break;
		case KEY_RESIZE: {
			get_screen_size();
			relayout();
		} break;
		default: {
			send_to_focus(ch);
		} break;
	}
	reap();
	return !_tabs.empty();
}

void UI::Shell::reap()
{
	// Closed windows go on the doomed list so they
	// don't actually get destroyed until the stack has
	// unwound. Otherwise, a window could request its
	// immediate destruction.
	while (!_doomed.empty()) {
		_doomed.pop();
	}
}

void UI::Shell::get_screen_size()
{
	getmaxyx(stdscr, _height, _width);
	_columnWidth = std::min(80, _width);
}

UI::Window *UI::Shell::open_window(std::unique_ptr<View> &&view, Position pos)
{
	// We reserve the top row for the title bar.
	// Aside from that, new windows fill the terminal rows.
	// Windows are never wider than 80 columns.
	Window *win = new Window(_app, std::move(view));
	size_t index;
	switch (pos) {
		case Position::Left: index = 0; break;
		case Position::Any: index = _focus + 1; break;
		case Position::Right: index = _tabs.size(); break;
	}
	_tabs.emplace(_tabs.begin() + index, win);
	relayout();
	set_focus(index);
	return win;
}

void UI::Shell::make_active(Window *window)
{
	for (unsigned i = 0; i < _tabs.size(); ++i) {
		if (_tabs[i].get() == window) {
			set_focus(i);
		}
	}
}

void UI::Shell::close_window(Window *window)
{
	for (unsigned i = 0; i < _tabs.size(); ++i) {
		if (_tabs[i].get() == window) {
			close_window(i);
		}
	}
}

void UI::Shell::set_focus(size_t index)
{
	assert(index >= 0 && index < _tabs.size());
	if (_focus < _tabs.size()) {
		_tabs[_focus]->clear_focus();
	}
	_focus = index;
	_tabs[_focus]->set_focus();
	// We want to keep as much of the background
	// visible as we can. This means we must stack
	// windows on the left of the focus in ascending
	// order, while windows to the right of the focus
	// are stacked in descending order. We raise the
	// focus window last.
	for (size_t i = 0; i < _focus; ++i) {
		_tabs[i]->bring_forward(-1);
	}
	for (size_t i = _tabs.size() - 1; i > _focus; --i) {
		_tabs[i]->bring_forward(1);
	}
	_tabs[_focus]->bring_forward(0);
}

void UI::Shell::relayout()
{
	// The leftmost window owns column zero and covers no more than 80
	// characters' width.
	// Divide any remaining space among any remaining windows and stagger
	// each remaining window proportionally across the screen.
	assert(_width >= _columnWidth);
	if(_tabs.empty()) return;
	size_t ubound = _tabs.size() - 1;
	int right_edge = _width - _columnWidth;
	_spacing = (ubound > 0) ? right_edge / ubound : 0;
	for (unsigned i = 0; i <= ubound; ++i) {
		int offset = (ubound - i) * _spacing;
		int xpos = (i > 0) ? right_edge - offset : 0;
		_tabs[i]->layout(xpos, _columnWidth);
	}
}

void UI::Shell::send_to_focus(int ch)
{
	bool more = _tabs[_focus]->process(ch);
	if (more) return;
	close_window(_focus);
	relayout();
}

void UI::Shell::close_window(size_t index)
{
	// If this window has focus, move focus first.
	// It will make everything simpler afterward.
	if (_focus == index) {
		if (index + 1 < _tabs.size()) {
			set_focus(index + 1);
		} else if (index > 0) {
			set_focus(index - 1);
		}
	}
	// Remove the window from the active list, but
	// don't delete it yet, because one of its
	// methods might be on our call stack.
	_doomed.emplace(std::move(_tabs[index]));
	_tabs.erase(_tabs.begin() + index);
	// If the current focus window's index is greater
	// than the index we just deleted, change the
	// index to its new, correct value.
	if (index < _focus) {
		_focus--;
	}
	relayout();
}
