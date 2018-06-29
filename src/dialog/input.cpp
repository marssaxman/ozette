// ozette
// Copyright (C) 2015-2018 Mars J. Saxman
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

#include <cstdlib>
#include "dialog/input.h"

Dialog::Input::Input(std::string value, Completer completer, Updater updater):
	_value(value),
	_completer(completer),
	_updater(updater),
	_cursor_pos(_value.size()),
	_anchor_pos(0) {
}

void Dialog::Input::process(UI::Frame &ctx, int ch) {
	std::string old_value = _value;
	switch (ch) {
		case Control::Cut: ctl_cut(ctx); break;
		case Control::Copy: ctl_copy(ctx); break;
		case Control::Paste: ctl_paste(ctx); break;
		case KEY_LEFT: arrow_left(ctx); break;
		case KEY_RIGHT: arrow_right(ctx); break;
		case KEY_SLEFT: select_left(ctx); break;
		case KEY_SRIGHT: select_right(ctx); break;
		case Control::Backspace: delete_prev(ctx); break;
		case KEY_DC: delete_next(ctx); break;
		case Control::Tab: tab_complete(ctx); break;
		default:
			// we only care about non-control chars now
			if (ch < 32 || ch > 127) break;
			// in all other situations, the keypress should be
			// inserted into the field at the cursor point.
			key_insert(ctx, ch);
			break;
	}
	if (_updater && _value != old_value) {
		_updater(ctx);
	}
}

void Dialog::Input::paint(
		WINDOW *view, int v, int h, int width, UI::View::State state) {
	// Move to the specified window location and draw the value, truncated
	// to fit in the available space.
	mvwaddnstr(view, v, h, _value.c_str(), width);
	// If there is empty space remaining, clear it out.
	int remaining = width - _value.size();
	if (remaining > 0) {
		whline(view, ' ', remaining);
	}

	// Position the cursor, or draw the selection range.
	bool focused = (state == UI::View::State::Focused);
	if (_anchor_pos == _cursor_pos) {
		// Put the cursor where it ought to be. Make it visible, if that
		// would be appropriate for our activation state.
		wmove(view, v, h + _cursor_pos);
		curs_set(focused? 1: 0);
	} else {
		if (focused) {
			int begin = h + std::min(_cursor_pos, _anchor_pos);
			int count = std::abs(static_cast<int>(_cursor_pos - _anchor_pos));
			mvwchgat(view, v, begin, count, A_NORMAL, 0, NULL);
		}
		curs_set(0);
	}
}

void Dialog::Input::set_help(UI::HelpBar::Panel &panel) {
	panel.cut();
	panel.copy();
	panel.paste();
}

void Dialog::Input::select_all(UI::Frame &ctx) {
	if (_anchor_pos != 0) {
		_anchor_pos = 0;
		ctx.repaint();
	}
	if (_cursor_pos != _value.size()) {
		_cursor_pos = _value.size();
		ctx.repaint();
	}
}

void Dialog::Input::ctl_cut(UI::Frame &ctx) {
	ctl_copy(ctx);
	delete_selection(ctx);
}

void Dialog::Input::ctl_copy(UI::Frame &ctx) {
	// Don't replace the current clipboard contents unless something is
	// actually selected.
	if (_anchor_pos == _cursor_pos) {
		return;
	}
	unsigned begin = std::min(_anchor_pos, _cursor_pos);
	unsigned count = std::max(_anchor_pos - begin, _cursor_pos - begin);
	ctx.app().set_clipboard(_value.substr(begin, count));
}

void Dialog::Input::ctl_paste(UI::Frame &ctx) {
	delete_selection(ctx);
	std::string clip = ctx.app().get_clipboard();
	_value = _value.substr(0, _cursor_pos) + clip + _value.substr(_cursor_pos);
	_cursor_pos += clip.size();
	_anchor_pos = _cursor_pos;
}

void Dialog::Input::arrow_left(UI::Frame &ctx) {
	if (_cursor_pos != _anchor_pos) {
		_cursor_pos = std::min(_cursor_pos, _anchor_pos);
		_anchor_pos = _cursor_pos;
		ctx.repaint();
	} else {
		select_left(ctx);
		_anchor_pos = _cursor_pos;
	}
}

void Dialog::Input::arrow_right(UI::Frame &ctx) {
	if (_cursor_pos != _anchor_pos) {
		_cursor_pos = std::max(_cursor_pos, _anchor_pos);
		_anchor_pos = _cursor_pos;
		ctx.repaint();
	} else {
		select_right(ctx);
		_anchor_pos = _cursor_pos;
	}
}

void Dialog::Input::select_left(UI::Frame &ctx) {
	if (_cursor_pos > 0) {
		_cursor_pos--;
		ctx.repaint();
	}
}

void Dialog::Input::select_right(UI::Frame &ctx) {
	if (_cursor_pos < _value.size()) {
		_cursor_pos++;
		ctx.repaint();
	}
}

void Dialog::Input::delete_prev(UI::Frame &ctx) {
	if (_cursor_pos == _anchor_pos) {
		select_left(ctx);
	}
	delete_selection(ctx);
}

void Dialog::Input::delete_next(UI::Frame &ctx) {
	if (_cursor_pos == _anchor_pos) {
		select_right(ctx);
	}
	delete_selection(ctx);
}

void Dialog::Input::delete_selection(UI::Frame &ctx) {
	if (_value.empty()) return;
	if (_cursor_pos == _anchor_pos) return;
	auto begin = std::min(_anchor_pos, _cursor_pos);
	auto end = std::max(_anchor_pos, _cursor_pos);
	_value = _value.substr(0, begin) + _value.substr(end);
	_cursor_pos = begin;
	_anchor_pos = begin;
	ctx.repaint();
}

void Dialog::Input::tab_complete(UI::Frame &ctx) {
	// If we have an autocompleter function, give it a chance to extend the
	// text to the left of the insertion point.
	if (!_completer) return;
	size_t searchpos = std::min(_cursor_pos, _anchor_pos);
	std::string prefix = _value.substr(0, searchpos);
	std::string postfix = _value.substr(searchpos);
	std::string extended = _completer(prefix);
	if (extended == prefix) return;
	_value = extended + postfix;
	_cursor_pos = extended.size();
	_anchor_pos = extended.size();
	ctx.repaint();
}

void Dialog::Input::key_insert(UI::Frame &ctx, int ch) {
	delete_selection(ctx);
	_value.insert(_cursor_pos++, 1, ch);
	_anchor_pos = _cursor_pos;
	ctx.repaint();
}

