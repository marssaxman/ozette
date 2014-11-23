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

#include "line.h"

const unsigned Editor::kTabWidth = 4;

unsigned Editor::Line::width() const
{
	return column(size());
}

Editor::column_t Editor::Line::column(offset_t loc) const
{
	column_t out = 0;
	offset_t pos = 0;
	for (auto ch: text()) {
		if (++pos > loc) break;
		advance(ch, out);
	}
	return out;
}

Editor::offset_t Editor::Line::offset(column_t h) const
{
	offset_t out = 0;
	column_t pos = 0;
	for (char ch: text()) {
		advance(ch, pos);
		if (pos > h) break;
		out++;
	}
	return out;
}

void Editor::Line::advance(char ch, column_t &h) const
{
	do {
		h++;
	} while (ch == '\t' && h % kTabWidth);
}

void Editor::Line::paint(WINDOW *dest, column_t hoff, unsigned width) const
{
	column_t h = 0;
	width += hoff;
	for (char ch: text()) {
		if (h == width) break;
		if (ch != '\t') {
			if (h >= hoff) waddch(dest, ch);
			h++;
		} else do {
			if (h >= hoff) waddch(dest, ' ');
			h++;
		} while (h < width && 0 != h % kTabWidth);
	}
	if (h < width) {
		wclrtoeol(dest);
	}
}

