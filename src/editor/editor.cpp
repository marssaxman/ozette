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
		case ERR: return true; // polling; we don't care
		case KEY_DOWN: key_down(false); break;
		case KEY_UP: key_up(false); break;
		case KEY_LEFT: key_left(false); break;
		case KEY_RIGHT: key_right(false); break;
		case KEY_NPAGE: key_page_down(); break;
		case KEY_PPAGE: key_page_up(); break;
		case KEY_HOME: key_home(); break; // move to beginning of line
		case KEY_END: key_end(); break; // move to end of line
		case KEY_SF: key_down(true); break; // shifted down-arrow
		case KEY_SR: key_up(true); break; // shifted up-arrow
		case KEY_SLEFT: key_left(true); break;
		case KEY_SRIGHT: key_right(true); break;
		case 127: key_backspace(); break;
		case KEY_DC: key_delete(); break;
		case '\r': key_return(); break;
		case '\n': key_enter(); break;
		default:
		if (ch >= 32 && ch < 127) key_insert(ch);
		else {
			ctx.set_status(std::to_string(ch));
			return true;
		}
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

void Editor::Controller::key_up(bool extend)
{
	_cursor.up(1);
	adjust_selection(extend);
}

void Editor::Controller::key_down(bool extend)
{
	_cursor.down(1);
	adjust_selection(extend);
}

void Editor::Controller::key_left(bool extend)
{
	_cursor.left();
	adjust_selection(extend);
}

void Editor::Controller::key_right(bool extend)
{
	_cursor.right();
	adjust_selection(extend);
}

void Editor::Controller::key_page_up()
{
	_cursor.up(_halfheight);
	drop_selection();
}

void Editor::Controller::key_page_down()
{
	_cursor.down(_halfheight);
	drop_selection();
}

void Editor::Controller::key_home()
{
	_cursor.home();
	drop_selection();
}

void Editor::Controller::key_end()
{
	_cursor.end();
	drop_selection();
}

void Editor::Controller::delete_selection()
{
	if (_selection.empty()) return;
	_cursor.move_to(_doc.erase(_selection));
	_update.forward(_cursor.location());
	drop_selection();
}

void Editor::Controller::key_insert(char ch)
{
	delete_selection();
	_cursor.move_to(_doc.insert(_cursor.location(), ch));
}

void Editor::Controller::key_backspace()
{
	if (_selection.empty()) key_left(true);
	delete_selection();
}

void Editor::Controller::key_delete()
{
	if (_selection.empty()) key_right(true);
	delete_selection();
}

void Editor::Controller::key_return()
{
	// Split the line at the cursor position and move the cursor to the new line.
	delete_selection();
	_cursor.move_to(_doc.split(_cursor.location()));
	_update.forward(_cursor.location());
}

void Editor::Controller::key_enter()
{
	// Split the line at the cursor position, but don't move the cursor.
	delete_selection();
	_doc.split(_cursor.location());
	_update.forward(_cursor.location());
}

void Editor::Controller::drop_selection()
{
	// The selection is no longer interesting. Move the anchor to the
	// current cursor location and reset the selection around it.
	_anchor = _cursor.location();
	_selection.reset(_anchor);
}

void Editor::Controller::adjust_selection(bool extend)
{
	if (extend) {
		// The cursor has moved in range-selection mode.
		// Leave the anchor where it is, then extend the
		// selection to include the new cursor point.
		_selection.extend(_anchor, _cursor.location());
	} else {
		// The cursor moved but did not extend the selection.
		drop_selection();
	}
}
