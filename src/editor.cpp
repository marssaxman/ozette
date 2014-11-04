#include "editor.h"
#include <fstream>
#include "view.h"

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
	size_t effscroll = std::min(_maxscroll, _scrollpos);
	for (unsigned i = 0; i < _height; ++i) {
		size_t line = i + effscroll;
		if (!_update.is_dirty(line)) continue;
		wmove(dest, (int)i, 0);
		if (line < _lines.size()) {
			waddnstr(dest, _lines[line].c_str(), _width);
		}
		wclrtoeol(dest);
	}
	_update.reset();
}

bool Editor::process(WINDOW *dest, int ch, App &app)
{
	update_dimensions(dest);
	switch (ch) {
		case KEY_DOWN: cursor_vert(1); break;
		case KEY_UP: cursor_vert(-1); break;
		case KEY_LEFT: arrow_left(); break;
		case KEY_RIGHT: arrow_right(); break;
		case 338: cursor_vert(_halfheight); break;
		case 339: cursor_vert(-_halfheight); break;
		default: break;
	}
	if (_update.has_dirty()) {
		paint(dest, true);
	}
	return true;
}

void Editor::arrow_left()
{
}

void Editor::arrow_right()
{
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

bool Editor::line_visible(size_t index) const
{
	return index >= _scrollpos && (index - _scrollpos) < _height;
}

void Editor::reveal_cursor()
{
	// If the cursor is already on screen, do nothing.
	if (line_visible(_cursy)) return;
	// Try to center the viewport over the cursor.
	if (_cursy < _scrollpos) {
		_scrollpos = _cursy - std::min(_cursy, _halfheight);
	}
	// Don't scroll so far we reveal empty space.
	_scrollpos = std::min(_scrollpos, _maxscroll);
	_update.all();
}

void Editor::cursor_vert(int delta)
{
	size_t cursy = _cursy;
	if (delta > 0) {
		cursy = std::min(cursy + 1, _maxscroll);
	} else if (cursy > 0) {
		cursy += delta;
	}
	if (cursy != _cursy) {
		_update.range(_cursy, cursy);
		reveal_cursor();
	}
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
	size_t newmax = linecount > _height ? linecount - _height : 0;
	if (newmax != _maxscroll) {
		_maxscroll = newmax;
		_update.all();
	}
}
