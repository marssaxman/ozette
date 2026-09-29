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

#include "text/layout.h"
#include "text/utf8.h"
#include <algorithm>
#include <limits>
#include <wchar.h>

Text::LineLayout::LineLayout(const std::string &text, unsigned tab_width):
		_length(text.size()) {
	tab_width = std::max(1U, tab_width);
	bool can_combine = false;
	for (size_t offset = 0; offset < text.size();) {
		auto decoded = UTF8::decode(text, offset);
		char32_t value = decoded.value;
		unsigned width;
		if (value == '\t') {
			width = tab_width - _width % tab_width;
			can_combine = false;
		} else {
			int cells = wcwidth(static_cast<wchar_t>(value));
			// Controls and unattached combining marks must not move the terminal
			// cursor unpredictably. Show a single replacement cell instead.
			if (value < 32 || (value >= 0x7F && value < 0xA0) ||
					cells < 0 || (cells == 0 && !can_combine)) {
				value = '?';
				cells = 1;
			}
			width = cells;
			can_combine = true;
		}
		width = std::min(width, std::numeric_limits<unsigned>::max() - _width);
		_characters.push_back({offset, offset + decoded.length, _width, width, value});
		_width += width;
		offset += decoded.length;
	}
}

unsigned Text::LineLayout::column(size_t offset) const {
	auto found = std::upper_bound(_characters.begin(), _characters.end(), offset,
		[](size_t offset, const Character &ch) { return offset < ch.end; });
	return found == _characters.end()? _width: found->column;
}

size_t Text::LineLayout::offset(unsigned column) const {
	if (column >= _width) return _length;
	auto found = std::upper_bound(_characters.begin(), _characters.end(), column,
		[](unsigned column, const Character &ch) { return column < ch.column; });
	return found == _characters.begin()? 0: (--found)->begin;
}

Text::LineLayout::Span Text::LineLayout::span(size_t begin, size_t end) const {
	if (begin >= end || begin >= _length) return {};
	auto first = std::upper_bound(_characters.begin(), _characters.end(), begin,
		[](size_t offset, const Character &ch) { return offset < ch.end; });
	auto last = std::lower_bound(_characters.begin(), _characters.end(), end,
		[](const Character &ch, size_t offset) { return ch.begin < offset; });
	--last;
	while (first != _characters.begin() && first->width == 0) --first;
	while (last != _characters.begin() && last->width == 0) --last;
	return {first->column, last->column + last->width};
}
