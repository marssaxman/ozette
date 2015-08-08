//
// ozette
// Copyright (C) 2014-2015 Mars J. Saxman
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

#include "ui/view.h"
#include "ui/colors.h"

UI::View::View():
	_window(newwin(0, 0, 0, 0)),
	_panel(new_panel(_window))
{
}

UI::View::~View()
{
	del_panel(_panel);
	delwin(_window);
}

void UI::View::layout(int vpos, int hpos, int height, int width)
{
	int old_height, old_width;
	getmaxyx(_window, old_height, old_width);
	int old_vpos, old_hpos;
	getbegyx(_window, old_vpos, old_hpos);
	if (old_height != height || old_width != width) {
		WINDOW *replacement = newwin(height, width, vpos, hpos);
		replace_panel(_panel, replacement);
		delwin(_window);
		_window = replacement;
	} else if (old_vpos != vpos || old_hpos != hpos) {
		move_panel(_panel, vpos, hpos);
	}
}

void UI::View::bring_forward()
{
	top_panel(_panel);
}

void UI::View::paint(State state)
{
	wmove(_window, 0, 0);
	curs_set(0);
	wattrset(_window, Colors::content(state != State::Inactive));
	paint_into(_window, state);
	wstandend(_window);
}

void UI::View::overlay_result(std::string message, State state)
{
	int cury, curx;
	getyx(_window, cury, curx);
	int numchars = message.size();
	int labelwidth = 2 + numchars + 2;
	int winheight, winwidth;
	getmaxyx(_window, winheight, winwidth);
	if (labelwidth > winwidth) {
		labelwidth = winwidth;
		numchars = labelwidth - 4;
	}
	int voff = winheight - 1;
	int hoff = (winwidth - labelwidth) / 2;
	wmove(_window, voff, hoff);
	auto color = Colors::chrome(state != State::Inactive);
	wattron(_window, A_REVERSE | color);
	waddstr(_window, "[ ");
	waddnstr(_window, message.c_str(), numchars);
	waddstr(_window, " ]");
	wattroff(_window, A_REVERSE | color);
	wmove(_window, cury, curx);
}

