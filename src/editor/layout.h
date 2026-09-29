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

#ifndef EDITOR_LAYOUT_H
#define EDITOR_LAYOUT_H

#include <vector>
#include "editor/coordinates.h"

namespace Editor {
class LineLayout {
public:
	struct Character {
		offset_t begin, end;
		column_t column, width;
		char32_t value;
	};
	LineLayout(const std::string &text, unsigned tab_width);
	// Interior bytes map to their character's beginning. Columns inside a tab
	// or wide character map to its beginning; equal columns skip combining marks.
	column_t column(offset_t offset) const;
	offset_t offset(column_t column) const;
	column_t width() const { return _width; }
	const std::vector<Character> &characters() const { return _characters; }
private:
	std::vector<Character> _characters;
	offset_t _length;
	column_t _width = 0;
};
} // namespace Editor

#endif // EDITOR_LAYOUT_H
