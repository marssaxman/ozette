#include "cursor.h"

Editor::Cursor::Cursor(Document &doc, Update &update):
	_doc(doc),
	_update(update)
{
}

void Editor::Cursor::move_up(size_t lines)
{
	if (_line > 0) {
		// Move up by the specified number of
		// lines, stopping at the beginning.
		_update.line(_line);
		_line -= std::min(_line, lines);
		_update.line(_line);
		_char = _doc.line(_line).offset(_col);
	} else move_home();
}

void Editor::Cursor::move_down(size_t lines)
{
	if (_line < _doc.maxline()) {
		// Move down by the specified number of
		// lines, stopping at the end.
		_update.line(_line);
		size_t newline = _line + lines;
		_line = std::min(newline, _doc.maxline());
		_update.line(_line);
		_char = _doc.line(_line).offset(_col);
	} else move_end();
}

void Editor::Cursor::move_left()
{
	if (_char > 0) {
		// Move one character left.
		_char--;
		_col = _doc.line(_line).column(_char);
		_update.line(_line);
	} else if (_line > 0) {
		// Wrap around to the end of the previous line.
		_update.line(_line);
		_line--;
		_update.line(_line);
		move_end();
	}
}

void Editor::Cursor::move_right()
{
	size_t linesize = _doc.line(_line).size();
	if (_char < linesize) {
		// Move one character right.
		_char++;
		_col = _doc.line(_line).column(_char);
		_update.line(_line);
	} else if (_line < _doc.maxline()) {
		// Wrap around to the beginning of the next line.
		_update.line(_line);
		_line++;
		_update.line(_line);
		move_home();
	}
}

void Editor::Cursor::move_home()
{
	size_t newchar = 0;
	unsigned newcol = 0;
	if (newchar != _char || newcol != _col) {
		_char = newchar;
		_col = newcol;
		_update.line(_line);
	}
}

void Editor::Cursor::move_end()
{
	size_t newchar = _doc.line(_line).size();
	unsigned newcol = _doc.line(_line).column(newchar);
	if (newchar != _char || newcol != _col) {
		_char = newchar;
		_col = newcol;
		_update.line(_line);
	}
}

