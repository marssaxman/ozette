#include "editor.h"
#include <fstream>
#include "view.h"

const unsigned Editor::Controller::kTabWidth = 4;

Editor::Controller::Controller(std::string targetpath):
	_targetpath(targetpath)
{
	std::string str;
	std::ifstream file(targetpath);
	while (std::getline(file, str)) {
		_lines.push_back(str);
	}
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
	for (char ch: get_line_text(index)) {
		if (column >= _width) break;
		if (ch != '\t') {
			waddch(dest, ch);
			column++;
		} else {
			waddch(dest, ACS_BULLET);
			while (++column % kTabWidth) {
				waddch(dest, ' ');
			}
		}
	}
	wclrtoeol(dest);
	if (_curs_line == index) {
		column = column_for_char(_curs_char, _curs_line);
		mvwchgat(dest, i, column, 1, A_REVERSE, 0, NULL);
	}
}

bool Editor::Controller::line_is_visible(size_t index) const
{
	return index >= _scrollpos && (index - _scrollpos) < _height;
}

std::string Editor::Controller::get_line_text(size_t index) const
{
	// get the specified line, if it is in range, or the empty string
	// if that is what we should display instead
	return index < _lines.size() ? _lines[index] : "";
}

size_t Editor::Controller::get_line_size(size_t index) const
{
	// return the line's length in chars, or 0 if out of range
	return index < _lines.size() ? _lines[index].size() : 0;
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
		_curs_char = char_for_column(_curs_col, _curs_line);
	} else move_cursor_home();
	reveal_cursor();
}

void Editor::Controller::move_cursor_down(size_t lines)
{
	if (_curs_line < _maxline) {
		// Move down by the specified number of
		// lines, stopping at the end.
		_update.line(_curs_line);
		size_t newline = _curs_line + lines;
		_curs_line = std::min(newline, _maxline);
		_update.line(_curs_line);
		_curs_char = char_for_column(_curs_col, _curs_line);
	} else move_cursor_end();
	reveal_cursor();
}

void Editor::Controller::move_cursor_left()
{
	if (_curs_char > 0) {
		// Move one character left.
		_curs_char--;
		_curs_col = column_for_char(_curs_char, _curs_line);
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
	size_t linesize = get_line_size(_curs_line);
	if (_curs_char < linesize) {
		// Move one character right.
		_curs_char++;
		_curs_col = column_for_char(_curs_char, _curs_line);
		_update.line(_curs_line);
	} else if (_curs_line < _maxline) {
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
	size_t newchar = get_line_size(_curs_line);
	unsigned newcol = column_for_char(newchar, _curs_line);
	if (newchar != _curs_char || newcol != _curs_col) {
		_curs_char = newchar;
		_curs_col = newcol;
		_update.line(_curs_line);
	}
	reveal_cursor();
}

size_t Editor::Controller::char_for_column(unsigned column, size_t line) const
{
	// Given a screen column coordinate and a line number,
	// compute the character offset which most closely
	// precedes that column.
	std::string text = get_line_text(line);
	column = std::min(column, _width);
	size_t charoff = 0;
	unsigned xpos = 0;
	for (auto ch: text) {
		xpos += char_width(ch, xpos);
		if (xpos >= column) break;
		charoff++;
	}
	return charoff;
}

unsigned Editor::Controller::column_for_char(size_t charoff, size_t line) const
{
	// Given a character offset and a line number, compute
	// the screen column coordinate where that character
	// should appear.
	std::string text = get_line_text(_curs_line);
	charoff = std::min(charoff, text.size());
	unsigned column = 0;
	size_t charpos = 0;
	for (auto ch: text) {
		charpos++;
		if (charpos >= charoff) break;
		column += char_width(ch, column);
	}
	return column;
}

unsigned Editor::Controller::char_width(char ch, size_t column) const
{
	// How many columns wide is this character, when
	// it appears at the specified column?
	return (ch == '\t') ? tab_width(column) : 1;
}

unsigned Editor::Controller::tab_width(size_t column) const
{
	// How many columns wide is a tab character
	// which begins at the specified column?
	return (column + kTabWidth) % kTabWidth;
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
	size_t linecount = _lines.size();
	_maxline = (linecount > 0) ? (linecount - 1) : 0;
	size_t newmax = linecount > _height ? linecount - _height : 0;
	if (newmax != _maxscroll) {
		_maxscroll = newmax;
		_update.all();
	}
}

