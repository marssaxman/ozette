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

void Editor::Cursor::move_up(size_t count)
{
	// Move up the screen by the specified number of rows,
	// stopping when we reach zero. Do not move the column.
	// If the cursor was already positioned on the top row,
	// move the cursor left to the beginning of the line.
	_update.at(_location);
	row_t newrow = _position.v - std::min(_position.v, count);
	if (newrow != _position.v) {
		_position.v = newrow;
	} else {
		_position.h = 0;
	}
	_location = _doc.location(_position);
	_update.at(_location);
}

void Editor::Cursor::move_down(size_t count)
{
	// Move down the screen by the specified number of rows,
	// stopping when we are on the maximum row. Do not move
	// the column. If the cursor was already positioned on
	// the maximum row, move the cursor right to the end of
	// the line.
	_update.at(_location);
	row_t row = std::min(_position.v + count, _doc.maxline());
	if (row != _position.v) {
		_position.v = row;
	} else {
		_position.h = UINT_MAX;
	}
	_location = _doc.location(_position);
	_update.at(_location);
}

void Editor::Cursor::move_left()
{
	// If the cursor is not located at the beginning of the
	// line, move it left by one character. If the cursor
	// was already at the beginning of the line, and the
	// line is not the beginning of the document, move the
	// cursor to the end of the previous line.
	_update.at(_location);
	if (_location.offset > 0) {
		_location.offset--;
	} else if (_location.line > 0) {
		_location.line--;
		_location.offset = _doc.line(_location.line).size();
	}
	_update.at(_location);
	_position = _doc.position(_location);
}

void Editor::Cursor::move_right()
{
	// If the cursor is not located at the end of the line,
	// move it right by one character. If the cursor was already
	// at the end of the line, and the line is not the last line
	// in the document, move the cursor to the beginning of the
	// next line.
	_update.at(_location);
	if (_location.offset < _doc.line(_location.line).size()) {
		_location.offset++;
	} else if (_location.line < _doc.maxline()) {
		_location.line++;
		_location.offset = 0;
	}
	_update.at(_location);
	_position = _doc.position(_location);
}

