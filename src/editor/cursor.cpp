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

#include "editor/cursor.h"
#include <climits>

// A cursor is a navigation implement with some semantics
// defined in terms of screen space and others in terms of
// document locations. This is a bit confusing, but these
// are behaviors with decades of history, and users expect
// things to work in a certain way, so we just have to deal.

Editor::Cursor::Cursor(Document &doc, Update &update):
	_doc(doc),
	_update(update) {
}

void Editor::Cursor::up() {
	// Move up the screen by the specified number of rows,
	// stopping when we reach zero. Do not move the column.
	// If the cursor was already positioned on the top row,
	// move the cursor left to the beginning of the line.
	begin_move();
	if (_position.v) {
		_position.v--;
		commit_position();
	} else {
		_location = _doc.home();
		commit_location();
	}
}

void Editor::Cursor::down() {
	// Move to the next row down the screen, stopping at the maximum row. 
	// Do not move the column. If the cursor was already positioned on the
	// maximum row, move the cursor right to the end of the line.
	begin_move();
	if (_position.v < _doc.maxline()) {
		_position.v++;
		commit_position();
	} else {
		_location = _doc.end();
		commit_location();
	}
}

void Editor::Cursor::left() {
	// Move left by one character. This may wrap us around to the end of the
	// previous line. If we are now positioned on a space character which is
	// not aligned to a tab stop, and the previous character is also a space,
	// move left again. Thus we move across indentations identically whether
	// they are composed of tab or space characters.
	location_t begin = _location;
	move_to(_doc.prev(_location));
	while (_position.h % kTabWidth) {
		location_t pprev = _doc.prev(_location);
		if ("  " != _doc.text(Range(pprev, begin))) return;
		begin = _location;
		move_to(pprev);
	}
}

void Editor::Cursor::right() {
	// Move right by one character. This may wrap around to the beginning of
	// the next line. If the target character was a space, keep moving until
	// we reach a non-space character or we reach tab-stop alignment.
	location_t begin = _location;
	move_to(_doc.next(_location));
	while (_position.h % kTabWidth) {
		location_t next = _doc.next(_location);
		if ("  " != _doc.text(Range(begin, next))) return;
		begin = _location;
		move_to(next);
	}
}

void Editor::Cursor::home() {
	// Put the cursor at the beginning of its line.
	begin_move();
	_location.offset = 0;
	commit_location();
}

void Editor::Cursor::end() {
	// Put the cursor at the end of its line.
	begin_move();
	_location.offset = _doc.line(_location.line).size();
	commit_location();
}

void Editor::Cursor::move_to(location_t loc) {
	// Place the cursor at an absolute document location.
	begin_move();
	_location = loc;
	commit_location();
}

void Editor::Cursor::begin_move() {
	// Mark the old location of the cursor as dirty so the
	// viewer will redraw that cell.
	_update.at(_location);
}

void Editor::Cursor::commit_location() {
	// We have updated the cursor's location in the document.
	// Tell the viewer what to redraw, then update the display
	// position according to the new location.
	_update.at(_location);
	_display = _position = _doc.position(_location);
}

void Editor::Cursor::commit_position() {
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
