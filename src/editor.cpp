#include "editor.h"
#include <fstream>
#include "view.h"

const unsigned Editor::kTabWidth = 4;

Editor::Editor(std::string targetpath):
	_targetpath(targetpath)
{
	std::string str;
	std::ifstream file(targetpath);
	while (std::getline(file, str)) {
		_lines.push_back(str);
	}
}

void Editor::paint(WINDOW *dest, bool active)
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

bool Editor::process(WINDOW *dest, int ch, App &app)
{
	update_dimensions(dest);
	switch (ch) {
		case KEY_DOWN: cursor_vert(1); break;
		case KEY_UP: cursor_vert(-1); break;
		case KEY_LEFT: cursor_horz(-1); break;
		case KEY_RIGHT: cursor_horz(1); break;
		case 338: cursor_vert(_halfheight); break;
		case 339: cursor_vert(-_halfheight); break;
		default: break;
	}
	if (_update.has_dirty()) {
		paint(dest, true);
	}
	return true;
}

void Editor::Update::reset()
{
	_dirty = false;
	_linestart = 0;
	_lineend = 0;
}

void Editor::Update::all()
{
	_dirty = true;
	_linestart = 0;
	_lineend = (size_t)-1;
}

void Editor::Update::line(size_t line)
{
	_linestart = _dirty ? std::min(_linestart, line) : line;
	_lineend = _dirty ? std::max(_lineend, line) : line;
	_dirty = true;
}

void Editor::Update::range(size_t a, size_t b)
{
	size_t from = std::min(a, b);
	size_t to = std::max(a, b);
	_linestart = _dirty ? std::min(_linestart, from) : from;
	_lineend = _dirty ? std::max(_linestart, to) : to;
	_dirty = true;
}

bool Editor::Update::is_dirty(size_t line) const
{
	return _dirty && (line >= _linestart) && (line <= _lineend);
}

void Editor::paint_line(WINDOW *dest, unsigned i)
{
	size_t index = i + _scrollpos;
	if (!_update.is_dirty(index)) return;
	wmove(dest, (int)i, 0);
	std::string text;
	if (index < _lines.size()) {
		text = _lines[index];
	}
	// We can't print this string unfiltered; we need to look
	// for tab characters and align them columnwise.
	size_t column = 0;
	int selcol = 0;
	for (char ch: text) {
		if (column <= _cursx) selcol = column;
		if (column >= _width) break;
		if (ch != '\t') {
			waddch(dest, ch);
			column++;
		} else do {
			waddch(dest, ACS_BULLET);
			if (++column >= _width) break;
		} while (0 != column % kTabWidth);
	}
	wclrtoeol(dest);
	if (_curs_line == index) {
		mvwchgat(dest, i, selcol, 1, A_REVERSE, 0, NULL);
	}
}

bool Editor::line_visible(size_t index) const
{
	return index >= _scrollpos && (index - _scrollpos) < _height;
}

unsigned Editor::line_columns(size_t index) const
{
	return (unsigned)column_for_char(index, (size_t)-1);
}

int Editor::column_for_char(size_t index, size_t xoff) const
{
	size_t column = 0;
	for (char ch: _lines[index]) {
		if (0 == xoff--) return column;
		if (column++ >= _width) break;
		if (ch != '\t') continue;
		while (0 != column % kTabWidth) {
			if (column++ >= _width) break;
		}
	}
	return column;
}

void Editor::reveal_cursor()
{
	// If the cursor is already on screen, do nothing.
	if (line_visible(_curs_line)) return;
	// Try to center the viewport over the cursor.
	_scrollpos = (_curs_line > _halfheight) ? (_curs_line - _halfheight) : 0;
	// Don't scroll so far we reveal empty space.
	_scrollpos = std::min(_scrollpos, _maxscroll);
	_update.all();
}

void Editor::cursor_vert(int delta)
{
	size_t curs_line = _curs_line;
	size_t cursx = _cursx;
	if (delta > 0) {
		curs_line += delta;
		if (curs_line > _maxline) {
			curs_line = _maxline;
			cursx = _lines[_maxline].size();
		}
	} else {
		if (curs_line >= (size_t)-delta) {
			curs_line += delta;
		} else {
			curs_line = 0;
			cursx = 0;
		}
	}
	// If the cursor is bouncing off its limits, do nothing.
	if (curs_line == _curs_line && cursx == _cursx) return;
	// Refresh the lines which have been changed and make sure
	// the cursor is visible on screen.
	_update.range(_curs_line, curs_line);
	_curs_line = curs_line;
	_cursx = cursx;
	reveal_cursor();
}

void Editor::cursor_horz(int delta)
{
	size_t cursx = _cursx;
	if (delta > 0) {
		cursx += delta;
		if (cursx > line_columns(_curs_line)) {
			cursx = 0;
			cursor_vert(1);
			return;
		}
	} else {
		if (cursx >= (size_t)-delta) {
			cursx += delta;
		} else {
			cursx = (size_t)-1;
			cursor_vert(-1);
			return;
		}
	}
	if (cursx == _cursx) return;
	_cursx = cursx;
	_update.line(_curs_line);
	reveal_cursor();
}

void Editor::update_dimensions(WINDOW *view)
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
