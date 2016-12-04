// ozette
// Copyright (C) 2014-2016 Mars J. Saxman
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

#include <algorithm>
#include <assert.h>
#include <string>
#include <vector>
#include "help/view.h"

extern const unsigned char HELP[];
extern unsigned int HELP_len;
static std::vector<std::string> helplines;
Help::View *Help::View::_instance;

void Help::View::show(UI::Shell &shell) {
	if (_instance) {
		shell.make_active(_instance->_window);
	} else {
		_instance = new Help::View;
		std::unique_ptr<UI::View> view(_instance);
		_instance->_window = shell.open_window(std::move(view));
	}
}

bool Help::View::process(UI::Frame &ctx, int ch) {
	switch (ch) {
		case Control::Close: return false;
		case KEY_UP: scrollby(-1, ctx); break;
		case KEY_DOWN: scrollby(1, ctx); break;
		case KEY_NPAGE: scrollby(_height/2, ctx); break;
		case KEY_PPAGE: scrollby(-_height/2, ctx); break;
	}
	return true;
}

Help::View::View() {
	assert(_instance == nullptr);
	if (helplines.empty()) {
		// Index the lines in the help text.
		const char *text = (const char*)HELP;
		const char *end = text + HELP_len;
		helplines.push_back(std::string());
		for (const char *s = text; s != end; ++s) {
			if (*s == '\n') {
				helplines.push_back(std::string(text, s - text));
				text = s+1;
			}
		}
		helplines.push_back(std::string(text, end - text));
	}
}

Help::View::~View() {
	_instance = nullptr;
}

void Help::View::paint_into(WINDOW *view, State state) {
	wmove(view, 0, 0);
	getmaxyx(view, _height, _width);
	for (int row = 0; row < _height; ++row) {
		wmove(view, row, 0);
		size_t i = row + _scrollpos;
		if (i > 0 && i < helplines.size()) {
			waddnstr(view, helplines[i].c_str(), _width);
		}
		wclrtoeol(view);
	}
}

void Help::View::scrollby(int delta, UI::Frame &ctx) {
	_scrollpos = ((int)_scrollpos + delta >= 0)? _scrollpos + delta: 0;
	int max = helplines.size() - _height / 2;
	if (max < 0) max = 0;
	if (_scrollpos > (unsigned)max) {
		_scrollpos = max;
	}
	ctx.repaint();
}

