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

#ifndef EDITOR_DISPLAYLINE_H
#define EDITOR_DISPLAYLINE_H

#include <string>
#include <ncurses.h>
#include <vector>
#include "editor/coordinates.h"
#include "editor/config.h"
#include "app/syntax.h"

namespace Editor {
class DisplayLine {
public:
	DisplayLine(
			const std::string &text,
			const Config &settings,
			const Syntax::Grammar &syntax);
	// Unformatted bytes
	const std::string &text() const { return _text; }
	// Number of bytes present
	size_t size() const { return _text.size(); }
	// Expected total width of rendered text
	unsigned width() const;
	// Get display column for some char offset
	column_t column(offset_t loc) const;
	// Get char offset for some display column
	offset_t offset(column_t h) const;
	// Render characters from location in buffer
	void paint(WINDOW *view, column_t hoff, unsigned width, bool active) const;
private:
	void advance(char ch, column_t &h) const;
	const std::string &_text;
	std::vector<int> _style;
	const Config &_config;
	const Syntax::Grammar &_syntax;
};
} // namespace Editor

#endif // EDITOR_DISPLAYLINE_H
