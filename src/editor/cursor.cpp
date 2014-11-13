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

#include "cursor.h"
#include <climits>

// A cursor is a navigation implement with some semantics
// defined in terms of screen space and others in terms of
// document locations. This is a bit confusing, but these
// are behaviors with decades of history, and users expect
// things to work in a certain way, so we just have to deal.

Editor::Cursor::Cursor(Document &doc, Update &update):
	_doc(doc),
	_update(update)
{
}

void Editor::Cursor::up(size_t count)
{
	// Move up the screen by the specified number of rows,
	// stopping when we reach zero. Do not move the column.
	// If the cursor was already positioned on the top row,
	// move the cursor left to the beginning of the line.
	begin_move();
	if (count <= _position.v) {
		_position.v -= count;
		commit_position();
	} else {
		_location = _doc.home();
		commit_location();
	}
}

void Editor::Cursor::down(size_t count)
{
	// Move down the screen by the specified number of rows,
	// stopping when we are on the maximum row. Do not move
	// the column. If the cursor was already positioned on
	// the maximum row, move the cursor right to the end of
	// the line.
	begin_move();
	auto maxv = _doc.maxline();
	if (_position.v + count <= maxv) {
		_position.v += count;
		commit_position();
	} else {
		_location = _doc.end();
		commit_location();
	}
}

void Editor::Cursor::left()
{
	move_to(_doc.prev(_location));
}

void Editor::Cursor::right()
{
	move_to(_doc.next(_location));
}

void Editor::Cursor::home()
{
	// Put the cursor at the beginning of its line.
	begin_move();
	_location.offset = 0;
	commit_location();
}

void Editor::Cursor::end()
{
	// Put the cursor at the end of its line.
	begin_move();
	_location.offset = _doc.line(_location.line).size();
	commit_location();
}

void Editor::Cursor::move_to(location_t loc)
{
	// Place the cursor at an absolute document location.
	begin_move();
	_location = loc;
	commit_location();
}

void Editor::Cursor::begin_move()
{
	// Mark the old location of the cursor as dirty so the
	// viewer will redraw that cell.
	_update.at(_location);
}

void Editor::Cursor::commit_location()
{
	// We have updated the cursor's location in the document.
	// Tell the viewer what to redraw, then update the display
	// position according to the new location.
	_update.at(_location);
	_display = _position = _doc.position(_location);
}

void Editor::Cursor::commit_position()
{
	// We have moved the cursor to a different screen position.
	// Update the location based on that position, tell the
	// viewer what it needs to redraw, then copy the position for
	// display. The display position is not the same as our
	// internal bookkeeping position, because we want to remember
	// what column the user began moving from even if the current
	// line does not actually have a character at that column.
	_location = _doc.location(_position);
	_update.at(_location);
	_display = _doc.position(_location);
}
