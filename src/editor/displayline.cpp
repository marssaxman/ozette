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

#include "editor/displayline.h"
#include "ui/colors.h"

Editor::DisplayLine::DisplayLine(
		const std::string &text,
		const Config &config,
		const Syntax::Grammar &syntax):
		_text(text),
		_style(text.size()),
		_config(config),
		_syntax(syntax) {
	static Regex trailing_space("[[:space:]]+$");
	Regex::Match m = trailing_space.find(text);
	if (!m.empty()) {
		for (size_t i= m.begin; i < m.end; ++i) {
			_style[i] = UI::Colors::error();
		}
	}
	for (auto &token: Syntax::parse(_syntax, text)) {
		for (size_t i = token.begin; i < token.end; ++i) {
			_style[i] = token.style();
		}
	}
}


unsigned Editor::DisplayLine::width() const {
	return column(size());
}

Editor::column_t Editor::DisplayLine::column(offset_t loc) const {
	column_t out = 0;
	offset_t pos = 0;
	for (auto ch: text()) {
		if (++pos > loc) break;
		advance(ch, out);
	}
	return out;
}

Editor::offset_t Editor::DisplayLine::offset(column_t h) const {
	offset_t out = 0;
	column_t pos = 0;
	for (char ch: text()) {
		advance(ch, pos);
		if (pos > h) break;
		out++;
	}
	return out;
}

void Editor::DisplayLine::advance(char ch, column_t &h) const {
	do {
		h++;
	} while (ch == '\t' && h % _config.indent_size());
}

void Editor::DisplayLine::paint(
		WINDOW *d, column_t hoff, unsigned width, bool active) const {
	column_t h = 0;
	width += hoff;
	size_t i = 0;
	for (char ch: text()) {
		if (h == width) break;
		if (active) {
			wattrset(d, _style[i++]);
		}
		// If it's a normal character, just draw it. If it's a tab, draw a
		// bullet, then add spaces up til the next tab stop.
		if (ch != '\t') {
			if (h >= hoff) waddch(d, ch);
			h++;
		} else {
			chtype bullet = ACS_BULLET;
			do {
				if (h >= hoff) waddch(d, bullet);
				h++;
				bullet = ' ';
			} while (h < width && 0 != h % _config.indent_size());
		}
	}
	wattrset(d, UI::Colors::content(active));
	if (h < width) {
		wclrtoeol(d);
	}
}

