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
	position_t curs = _cursor.position();
	curs.v -= std::min(curs.v, _scrollpos);
	wmove(dest, curs.v, curs.h);
	curs_set(active ? 1 : 0);
	_update.reset();
}

bool Editor::Controller::process(Context &ctx, int ch)
{
	switch (ch) {
		case KEY_DOWN: _cursor.down(1); clear_sel(); break;
		case KEY_UP: _cursor.up(1); clear_sel(); break;
		case KEY_LEFT: _cursor.left(); clear_sel(); break;
		case KEY_RIGHT: _cursor.right(); clear_sel(); break;
		case KEY_NPAGE: _cursor.down(_halfheight); clear_sel(); break;
		case KEY_PPAGE: _cursor.up(_halfheight); clear_sel(); break;
		case KEY_HOME: break; // move to beginning of line
		case KEY_END: break; // move to end of line
		case KEY_SF: _cursor.down(1); extend_sel(); break; // shifted down-arrow
		case KEY_SR: _cursor.up(1); extend_sel(); break; // shifted up-arrow
		case KEY_SLEFT: _cursor.left(); extend_sel(); break;
		case KEY_SRIGHT: _cursor.right(); extend_sel(); break;
		default: if (ch >= 32 && ch < 127) insert(ch); break;
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
	auto &line = _doc.line(index);
	line.paint(dest, _width);
	if (!active) return;
	if (_selection.empty()) return;
	column_t selbegin = 0;
	unsigned selcount = 0;
	if (_selection.begin().line < index && _selection.end().line > index) {
		selcount = _width;
	} else if (_selection.begin().line < index && _selection.end().line == index) {
		selcount = line.column(_selection.end().offset);
	} else if (_selection.begin().line == index && _selection.end().line > index) {
		selbegin = line.column(_selection.begin().offset);
		selcount = _width - selbegin;
	} else if (_selection.begin().line == index && _selection.end().line == index) {
		selbegin = line.column(_selection.begin().offset);
		selcount = line.column(_selection.end().offset) - selbegin;
	}
	if (selcount > 0) {
		mvwchgat(dest, v, selbegin, selcount, A_REVERSE, 0, NULL);
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

void Editor::Controller::clear_sel()
{
	// The cursor has moved as an insertion point.
	// If the selection was not empty, update
	// the affected lines.
	// Clear the selection and move the anchor
	// to the current cursor location.
	_sel_anchor = _cursor.location();
	_selection.reset(_sel_anchor);
}

void Editor::Controller::extend_sel()
{
	// The cursor has moved in range-selection mode.
	// Leave the anchor where it is, then extend the
	// selection to include the new cursor point.
	_selection.extend(_sel_anchor, _cursor.location());
}

void Editor::Controller::insert(char ch)
{
	location_t loc = _selection.begin();
	if (!_selection.empty()) {
		_doc.erase(_selection);
		_update.forward(loc);
	}
	_cursor.move_to(_doc.insert(loc, ch));
	clear_sel();
}
