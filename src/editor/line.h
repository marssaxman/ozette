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

#ifndef EDITOR_LINE_H
#define EDITOR_LINE_H

#include <string>
#include <ncurses.h>
#include "coordinates.h"

namespace Editor {
extern const unsigned kTabWidth;
class Line
{
public:
	Line(std::string text): _text(text) {}
	// Unformatted bytes
	std::string text() const { return _text; }
	// Number of bytes present
	size_t size() const { return _text.size(); }
	// Expected total width of rendered text
	unsigned width() const;
	// Get display column for some char offset
	column_t column(offset_t loc) const;
	// Get char offset for some display column
	offset_t offset(column_t h) const;
	// Render characters from location in buffer
	void paint(WINDOW *view, column_t hoff, unsigned width) const;
private:
	void advance(char ch, column_t &h) const;
	std::string _text;
};
} // namespace Editor

#endif // EDITOR_LINE_H
