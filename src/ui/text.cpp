// ozette
// Copyright (C) 2026 Mars J. Saxman
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

#include "ui/text.h"
#include <algorithm>

void UI::paint_text(WINDOW *dest, int v, int h, int width,
		const Editor::LineLayout &layout, Editor::column_t scroll, int normal,
		const std::vector<int> &styles, Editor::LineLayout::Span selection) {
	if (width <= 0) return;
	wattrset(dest, normal);
	mvwhline(dest, v, h, ' ', width);
	// Subtract the viewport origin before clipping, avoiding unsigned underflow
	// for selections which begin or end to the left of the visible text.
	unsigned selbegin = std::min<unsigned>(width, selection.begin - std::min(selection.begin, scroll));
	unsigned selend = std::min<unsigned>(width, selection.end - std::min(selection.end, scroll));
	if (selend > selbegin) {
		wattrset(dest, normal ^ A_REVERSE);
		mvwhline(dest, v, h + selbegin, ' ', selend - selbegin);
	}
	const auto &characters = layout.characters();
	for (size_t i = 0; i < characters.size();) {
		const auto &ch = characters[i++];
		wchar_t text[CCHARW_MAX + 1] = {static_cast<wchar_t>(ch.value)};
		size_t count = 1;
		while (i < characters.size() && characters[i].width == 0) {
			if (count < CCHARW_MAX) text[count++] = characters[i].value;
			++i;
		}
		if (ch.column + ch.width <= scroll) continue;
		unsigned left = ch.column - std::min(ch.column, scroll);
		if (left >= static_cast<unsigned>(width)) break;
		unsigned right = std::min<unsigned>(width, ch.column + ch.width - scroll);
		int style = ch.begin < styles.size()? styles[ch.begin]: normal;
		if (left < selend && right > selbegin) style = normal ^ A_REVERSE;
		wattrset(dest, style);
		if (ch.value == '\t' || ch.column < scroll || ch.width > right - left) {
			for (unsigned column = left; column < right; ++column) {
				chtype cell = ch.value == '\t' && ch.column >= scroll &&
					column == left? ACS_BULLET: ' ';
				mvwaddch(dest, v, h + column, cell);
			}
		} else {
			cchar_t cell;
			setcchar(&cell, text, style & ~A_COLOR, PAIR_NUMBER(style), nullptr);
			mvwadd_wch(dest, v, h + left, &cell);
		}
	}
	wattrset(dest, normal);
}
