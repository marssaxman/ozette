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
	// If the cursor is not located at the beginning of the
	// line, move it left by one character. If the cursor
	// was already at the beginning of the line, and the
	// line is not the beginning of the document, move the
	// cursor to the end of the previous line.
	begin_move();
	if (_location.offset > 0) {
		_location.offset--;
	} else if (_location.line > 0) {
		_location.line--;
		_location.offset = _doc.line(_location.line).size();
	}
	commit_location();
}

void Editor::Cursor::right()
{
	// If the cursor is not located at the end of the line,
	// move it right by one character. If the cursor was already
	// at the end of the line, and the line is not the last line
	// in the document, move the cursor to the beginning of the
	// next line.
	begin_move();
	if (_location.offset < _doc.line(_location.line).size()) {
		_location.offset++;
	} else if (_location.line < _doc.maxline()) {
		_location.line++;
		_location.offset = 0;
	}
	commit_location();
}

void Editor::Cursor::move_to(location_t loc)
{
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
