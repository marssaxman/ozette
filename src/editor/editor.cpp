#include "editor.h"

Editor::Controller::Controller(std::string targetpath):
	_targetpath(targetpath),
	_doc(targetpath)
{
}

void Editor::Controller::paint(WINDOW *dest, bool active)
{
	update_dimensions(dest);
	if (active != _last_active || dest != _last_dest) {
		_update.all();
		_last_active = active;
		_last_dest = dest;
	}
	for (unsigned i = 0; i < _height; ++i) {
		paint_line(dest, i);
	}
	_update.reset();
}

bool Editor::Controller::process(WINDOW *dest, int ch, App &app)
{
	update_dimensions(dest);
	switch (ch) {
		case KEY_DOWN: move_cursor_down(1); break;
		case KEY_UP: move_cursor_up(1); break;
		case KEY_LEFT: move_cursor_left(); break;
		case KEY_RIGHT: move_cursor_right(); break;
		case 338: move_cursor_down(_halfheight); break;
		case 339: move_cursor_up(_halfheight); break;
		default: break;
	}
	if (_update.has_dirty()) {
		paint(dest, true);
	}
	return true;
}

void Editor::Controller::paint_line(WINDOW *dest, unsigned i)
{
	size_t index = i + _scrollpos;
	if (!_update.is_dirty(index)) return;
	wmove(dest, (int)i, 0);
	// We can't print this string unfiltered; we need to look
	// for tab characters and align them columnwise.
	unsigned column = 0;
	for (char ch: _doc.get_line_text(index)) {
		if (column >= _width) break;
		if (ch != '\t') {
			waddch(dest, ch);
			column++;
		} else {
			waddch(dest, ACS_BULLET);
			while (++column % Document::kTabWidth) {
				waddch(dest, ' ');
			}
		}
	}
	wclrtoeol(dest);
	if (_curs_line == index) {
		column = _doc.column_for_char(_curs_char, _curs_line);
		mvwchgat(dest, i, column, 1, A_REVERSE, 0, NULL);
	}
}

bool Editor::Controller::line_is_visible(size_t index) const
{
	return index >= _scrollpos && (index - _scrollpos) < _height;
}

void Editor::Controller::reveal_cursor()
{
	// If the cursor is already on screen, do nothing.
	if (line_is_visible(_curs_line)) return;
	// Try to center the viewport over the cursor.
	_scrollpos = (_curs_line > _halfheight) ? (_curs_line - _halfheight) : 0;
	// Don't scroll so far we reveal empty space.
	_scrollpos = std::min(_scrollpos, _maxscroll);
	_update.all();
}

void Editor::Controller::move_cursor_up(size_t lines)
{
	if (_curs_line > 0) {
		// Move up by the specified number of
		// lines, stopping at the beginning.
		_update.line(_curs_line);
		_curs_line -= std::min(_curs_line, lines);
		_update.line(_curs_line);
		_curs_char = _doc.char_for_column(_curs_col, _curs_line);
	} else move_cursor_home();
	reveal_cursor();
}

void Editor::Controller::move_cursor_down(size_t lines)
{
	if (_curs_line < _doc.maxline()) {
		// Move down by the specified number of
		// lines, stopping at the end.
		_update.line(_curs_line);
		size_t newline = _curs_line + lines;
		_curs_line = std::min(newline, _doc.maxline());
		_update.line(_curs_line);
		_curs_char = _doc.char_for_column(_curs_col, _curs_line);
	} else move_cursor_end();
	reveal_cursor();
}

void Editor::Controller::move_cursor_left()
{
	if (_curs_char > 0) {
		// Move one character left.
		_curs_char--;
		_curs_col = _doc.column_for_char(_curs_char, _curs_line);
		_update.line(_curs_line);
	} else if (_curs_line > 0) {
		// Wrap around to the end of the previous line.
		_update.line(_curs_line);
		_curs_line--;
		_update.line(_curs_line);
		move_cursor_end();
	}
	reveal_cursor();
}

void Editor::Controller::move_cursor_right()
{
	size_t linesize = _doc.get_line_size(_curs_line);
	if (_curs_char < linesize) {
		// Move one character right.
		_curs_char++;
		_curs_col = _doc.column_for_char(_curs_char, _curs_line);
		_update.line(_curs_line);
	} else if (_curs_line < _doc.maxline()) {
		// Wrap around to the beginning of the next line.
		_update.line(_curs_line);
		_curs_line++;
		_update.line(_curs_line);
		move_cursor_home();
	}
	reveal_cursor();
}

void Editor::Controller::move_cursor_home()
{
	size_t newchar = 0;
	unsigned newcol = 0;
	if (newchar != _curs_char || newcol != _curs_col) {
		_curs_char = newchar;
		_curs_col = newcol;
		_update.line(_curs_line);
	}
	reveal_cursor();
}

void Editor::Controller::move_cursor_end()
{
	size_t newchar = _doc.get_line_size(_curs_line);
	unsigned newcol = _doc.column_for_char(newchar, _curs_line);
	if (newchar != _curs_char || newcol != _curs_col) {
		_curs_char = newchar;
		_curs_col = newcol;
		_update.line(_curs_line);
	}
	reveal_cursor();
}

void Editor::Controller::update_dimensions(WINDOW *view)
{
	int height, width;
	getmaxyx(view, height, width);
	if ((size_t)height != _height) {
		_height = (size_t)height;
		_halfheight = _height / 2;
		_update.all();
	}
	if ((size_t)width != _width) {
		_width = (size_t)width;
		_update.all();
	}
	size_t newmax = std::max(_doc.maxline(), _height) - _halfheight;
	if (newmax != _maxscroll) {
		_maxscroll = newmax;
		_scrollpos = std::min(_scrollpos, _maxscroll);
		_update.all();
	}
}

