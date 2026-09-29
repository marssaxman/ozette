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

#ifndef TEXT_LAYOUT_H
#define TEXT_LAYOUT_H

#include <cstddef>
#include <string>
#include <vector>

namespace Text {
class LineLayout {
public:
	struct Character {
		size_t begin, end;
		unsigned column, width;
		char32_t value;
	};
	struct Span {
		Span(unsigned a = 0, unsigned b = 0): begin(a), end(b) {}
		unsigned begin, end;
	};
	LineLayout(const std::string &text, unsigned tab_width);
	// Interior bytes map to their character's beginning. Columns inside a tab
	// or wide character map to its beginning; equal columns skip combining marks.
	unsigned column(size_t offset) const;
	size_t offset(unsigned column) const;
	// Highlight every cell touched by these bytes, including the base of a
	// selected combining mark and any partially selected multibyte character.
	Span span(size_t begin, size_t end) const;
	unsigned width() const { return _width; }
	const std::vector<Character> &characters() const { return _characters; }
private:
	std::vector<Character> _characters;
	size_t _length;
	unsigned _width = 0;
};
} // namespace Text

#endif // TEXT_LAYOUT_H
