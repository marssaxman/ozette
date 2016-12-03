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

#ifndef EDITOR_CURSOR_H
#define EDITOR_CURSOR_H

#include "editor/document.h"
#include "editor/update.h"
#include "editor/settings.h"

// A cursor points at a location in a document and
// navigates to other locations relative to it.
namespace Editor {
class Cursor {
public:
	Cursor(Document&, Update&, Settings&);
	const location_t location() const { return _location; }
	const position_t position() const { return _display; }
	void up();
	void down();
	void left();
	void right();
	void home();
	void end();
	void move_to(location_t loc);
private:
	void begin_move();
	void commit_location();
	void commit_position();
	// Convert back and forth between document and screen coordinates.
	position_t to_position(const location_t &in_document);
	location_t to_location(const position_t &on_display);
	Document &_doc;
	Update &_update;
	Settings &_settings;
	location_t _location;
	// We use this internal position record to keep the cursor
	// located on the same column when moving up and down, as
	// closely as we can approximate. The display position of
	// the cursor is a separate value, calculated back from
	// the location, so that it always corresponds to an actual
	// character (whereas the abstract position may point at a
	// cell which is not occupied by any actual character).
	position_t _position = {0,0};
	position_t _display = {0,0};
};
} // namespace Editor

#endif // EDITOR_CURSOR_H
