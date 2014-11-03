#include "listform.h"
#include <assert.h>

void ListForm::paint(WINDOW *view)
{
	if (_dirty) {
		_dirty = false;
		refresh();
	}
	int height, width;
	getmaxyx(view, height, width);
	for (int y = 0; y < height; ++y) {
		paint_line(view, y, height, width);
	}
}

bool ListForm::process(WINDOW *view, int ch)
{
        switch (ch) {
                case 258: arrow_down(view); break;
                case 259: arrow_up(view); break;
		case '\r': commit(view); break;
		case 28: escape(view); break;
		default: break;
	}
	return true;
}

bool ListForm::poll(WINDOW *view)
{
	return true;
}

void ListForm::refresh()
{
	// Render our commands and entries down to some text lines.
	_lines.clear();
	LineBuilder fields(_lines);
	fields.blank();
	render(fields);
	fields.blank();
	// If the cursor has gone out of range, bring it back.
	if (_selpos >= _lines.size()) {
		_selpos = _lines.size();
	}
	// If the cursor is not on a selectable line, look for
	// one further down it might apply to.
	if (!is_selectable(_selpos)) {
		size_t delta = 1;
		while (delta < _lines.size()) {
			if (is_selectable(_selpos + delta)) {
				_selpos += delta;
				break;
			}
			if (is_selectable(_selpos - delta)) {
				_selpos -= delta;
				break;
			}
			++delta;
		}
	}

}

void ListForm::paint_line(WINDOW *view, int y, int height, int width)
{
	size_t line = (size_t)y + _scrollpos;
	int lwid = std::max(0, width - 2);
	wmove(view, y, 0);
	if (line < _lines.size()) {
		std::string left = _lines[line].left_text;
		std::string right = _lines[line].right_text;
		int rchars = std::min((int)right.size(), lwid/2);
		int lchars = std::min((int)left.size(), lwid-rchars);
		waddch(view, ' ');
		waddnstr(view, left.c_str(), lchars);
		wclrtoeol(view);
		mvwaddnstr(view,y, width - 1 - rchars, right.c_str(), rchars);
	} else {
		wclrtoeol(view);
	}
	if (line == _selpos) {
		mvwchgat(view, y, 1, lwid, A_REVERSE, 0, NULL);
	}
}

bool ListForm::is_selectable(ssize_t line)
{
	if (line < 0) return false;
	if ((size_t)line >= _lines.size()) return false;
	return _lines[line].action != nullptr;
}

void ListForm::arrow_down(WINDOW *view)
{
	// Look for a selectable line past the current one.
	// If we find one, select it, then repaint.
	// If we don't find one, do nothing.
	for (size_t i = _selpos + 1; i < _lines.size(); ++i) {
		if (!is_selectable(i)) continue;
		_selpos = i;
		scroll_to_selection(view);
		break;
	}
}

void ListForm::arrow_up(WINDOW *view)
{
	// Look for a selectable line before the current one.
	// If we find one, select it, then repaint.
	// If we don't find one, leave the selection alone.
	for (size_t i = _selpos; i > 0; --i) {
		size_t next = i - 1;
		if (!is_selectable(next)) continue;
		_selpos = next;
		scroll_to_selection(view);
		break;
	}
}

void ListForm::commit(WINDOW *view)
{
	// Invoke the selected field.
	assert(_selpos < _lines.size());
	auto &line = _lines[_selpos];
	if (line.action) {
		line.action();
		_dirty = true;
		paint(view);
	}
}

void ListForm::escape(WINDOW *view)
{
	// If we had a text edit field open, close it.
}

void ListForm::scroll_to_selection(WINDOW *view)
{
	// If the selected item is not visible, adjust the scroll
	// position until it becomes visible, then repaint.
	int height, width;
	getmaxyx(view, height, width);
	(void)width; 	//unused
	size_t heightz = static_cast<size_t>(height);
	size_t half_page = heightz / 2;
	size_t first_visible = _scrollpos;
	size_t last_visible = _scrollpos + heightz;
	if (_selpos < first_visible) {
		_scrollpos = (_selpos > half_page) ? _selpos - half_page : 0;
	}
	if (_selpos >= last_visible) {
		size_t max_scroll = (_lines.size() > heightz) ? _lines.size() - heightz : 0;
		_scrollpos = std::min(max_scroll, _selpos - half_page);
	}

	// This function will ALWAYS repaint, whether or not it moved
	// the cursor, because it is intended for use after operations
	// which move the selection, and such operations require us to
	// repaint the window anyway.
	paint(view);
}

void ListForm::LineBuilder::entry(std::string text, std::function<void()> action)
{
	ListForm::Line line;
	size_t split = text.find_first_of('\t');
	if (split != std::string::npos) {
		line.left_text = text.substr(0, split);
		line.right_text = text.substr(split+1, std::string::npos);
	} else {
		line.left_text = text;
	}
	line.action = action;
	_lines.push_back(line);
}
