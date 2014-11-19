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
	virtual ~Line() = default;
	// Unformatted bytes
	virtual std::string text() const = 0;
	// Number of bytes present
	virtual size_t size() const;
	// Expected total width of rendered text
	virtual unsigned width();
	// Get display column for some char offset
	virtual column_t column(offset_t loc);
	// Get char offset for some display column
	virtual offset_t offset(column_t h);
	// Render characters from location in buffer
	virtual void paint(WINDOW *view, column_t hoff, unsigned width);
private:
	void advance(char ch, column_t &h);
};
} // namespace Editor

#endif // EDITOR_LINE_H
