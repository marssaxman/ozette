#include "editor.h"

Editor::Controller::Controller(std::string targetpath):
	_targetpath(targetpath),
	_doc(targetpath),
	_cursor(_doc, _update)
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
		paint_line(dest, i, active);
	}
	_update.reset();
}

bool Editor::Controller::process(Context &ctx, int ch)
{
	switch (ch) {
		case KEY_DOWN: _cursor.down(1); break;
		case KEY_UP: _cursor.up(1); break;
		case KEY_LEFT: _cursor.left(); break;
		case KEY_RIGHT: _cursor.right(); break;
		case 338: _cursor.down(_halfheight); break;
		case 339: _cursor.up(_halfheight); break;
		default: break;
	}
	reveal_cursor();
	if (_update.has_dirty()) {
		ctx.repaint();
	}
	return true;
}

void Editor::Controller::paint_line(WINDOW *dest, row_t v, bool active)
{
	size_t index = v + _scrollpos;
	if (!_update.is_dirty(index)) return;
	wmove(dest, (int)v, 0);
	// We can't print this string unfiltered; we need to look
	// for tab characters and align them columnwise.
	column_t h = 0;
	for (char ch: _doc.line(index).text()) {
		if (h >= _width) break;
		if (ch != '\t') {
			waddch(dest, ch);
			h++;
		} else while (++h % kTabWidth && h < _width) {
			waddch(dest, ' ');
		}
	}
	wclrtoeol(dest);
	if (active && _cursor.position().v == index) {
		h = _cursor.position().h;
		mvwchgat(dest, v, h, 1, A_REVERSE, 0, NULL);
	}
}

bool Editor::Controller::line_is_visible(size_t index) const
{
	return index >= _scrollpos && (index - _scrollpos) < _height;
}

void Editor::Controller::reveal_cursor()
{
	line_t line = _cursor.location().line;
	// If the cursor is already on screen, do nothing.
	if (line_is_visible(line)) return;
	// Try to center the viewport over the cursor.
	_scrollpos = (line > _halfheight) ? (line - _halfheight) : 0;
	// Don't scroll so far we reveal empty space.
	_scrollpos = std::min(_scrollpos, _maxscroll);
	_update.all();
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

