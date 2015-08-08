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

#include "browser/picker.h"

Browser::Picker::Picker(
		std::string prompt, std::vector<std::string> options, action_t commit):
	Input(prompt, commit)
{
	_options = options;
	_suggestion_selected = true;
	_sugg_item = 0;
	if (!_options.empty()) {
		_value = _options.front();
	}
}

Browser::Picker::Picker(std::string prompt, std::string value, action_t commit):
	Input(prompt, commit)
{
	_value = value;
	move_cursor(_value.size());
}

bool Browser::Picker::process(UI::Frame &ctx, int ch)
{
	switch (ch) {
		case KEY_LEFT: arrow_left(ctx); break;
		case KEY_RIGHT: arrow_right(ctx); break;
		case KEY_UP: arrow_up(); break;
		case KEY_DOWN: arrow_down(); break;
		default: {
			// if this is a digit character and the selection
			// is on the suggestion list, insta-commit the
			// item corresponding to that digit
			if (ch >= '0' && ch <= '9' && _suggestion_selected) {
				select_suggestion(ch - '0');
				if (_commit) _commit(ctx, _value);
				return false;
			}
			return inherited::process(ctx, ch);
		} break;
	}
	return true;
}

void Browser::Picker::paint_into(WINDOW *view, State state)
{
	inherited::paint_into(view, state);
	int height, width;
	getmaxyx(view, height, width);
	int old_ypos, old_xpos;
	getyx(view, old_ypos, old_xpos);

	// Draw each suggested value on its own line.
	int sugg_vpos = old_ypos + 1;
	int sugg_width = width;
	int sugg_hpos = 0;
	// Reserve two columns on each side as a margin.
	sugg_width -= 4;
	sugg_hpos += 2;
	// Reduce the field width by three more chars to give
	// space for the quick-select number captions.
	sugg_width -= 3;

	for (unsigned i = 0; i < _options.size(); ++i) {
		int vpos = sugg_vpos + i;
		if (vpos >= height) break;
		wmove(view, vpos, 0);
		if (i < 10 && _suggestion_selected) {
			waddstr(view, "  ");
			waddch(view, '0' + i);
			waddstr(view, ": ");
		} else {
			waddstr(view, "     ");
		}
		bool selrow = (_suggestion_selected && i == _sugg_item);
		if (selrow) {
			wattroff(view, A_REVERSE);
			if (vpos + 1 == height) wattron(view, A_UNDERLINE);
		}
		waddnstr(view, _options[i].c_str(), sugg_width);
		int curv, curh;
		getyx(view, curv, curh);
		(void)curv; //ignored
		whline(view, ' ', width - curh - 2);
		if (selrow) {
			wattron(view, A_REVERSE);
			if (vpos + 1 == height) wattroff(view, A_UNDERLINE);
		}
		mvwaddstr(view, vpos, width - 2, "  ");
	}
	wmove(view, old_ypos, old_xpos);
}

void Browser::Picker::select_field()
{
	inherited::select_field();
	_suggestion_selected = false;
}

bool Browser::Picker::field_selected() const
{
	return !_suggestion_selected;
}

void Browser::Picker::select_suggestion(size_t i)
{
	if (i >= _options.size()) return;
	if (_suggestion_selected && _sugg_item == i) return;
	_suggestion_selected = true;
	_sugg_item = i;
	repaint();
	std::string value = _options[i];
	set_value(value);
	move_cursor(value.size());
}

void Browser::Picker::arrow_left(UI::Frame &ctx)
{
	if (_suggestion_selected) {
		select_field();
		move_cursor(_value.size());
	} else {
		inherited::process(ctx, KEY_LEFT);
	}
}

void Browser::Picker::arrow_right(UI::Frame &ctx)
{
	if (_suggestion_selected) {
		select_field();
		move_cursor(0);
	} else if (cursor_pos() < _value.size()) {
		inherited::process(ctx, KEY_RIGHT);
	}
}

void Browser::Picker::arrow_up()
{
	if (!_suggestion_selected) return;
	if (_sugg_item > 0) {
		select_suggestion(_sugg_item - 1);
	} else {
		select_field();
	}
}

void Browser::Picker::arrow_down()
{
	if (_suggestion_selected) {
		select_suggestion(_sugg_item + 1);
	} else {
		select_suggestion(0);
	}
}

