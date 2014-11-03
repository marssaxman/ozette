#include "listform.h"
#include <assert.h>

namespace ListForm {

class Blank : public Field
{
public:
	virtual bool active() const override { return false; }
	virtual void paint(WINDOW *view, size_t width) override {}
};

} // namespace ListForm

void ListForm::Field::highlight(WINDOW *view, size_t width)
{
	wchgat(view, width, A_REVERSE, 0, NULL);
}

void ListForm::Controller::paint(WINDOW *view)
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

bool ListForm::Controller::process(WINDOW *view, int ch)
{
        switch (ch) {
                case KEY_DOWN: arrow_down(view); break;
                case KEY_UP: arrow_up(view); break;
		case '\r': commit(view); break;
		case 28: escape(view); break;
		default: break;
	}
	return true;
}

bool ListForm::Controller::poll(WINDOW *view)
{
	return true;
}

namespace {
class LineBuilder : public ListForm::Builder
{
public:
	LineBuilder(std::vector<std::unique_ptr<ListForm::Field>> &lines): _lines(lines) {}
	virtual void add(std::unique_ptr<ListForm::Field> &&field) override
	{
		_lines.emplace_back(std::move(field));
	}
private:
	std::vector<std::unique_ptr<ListForm::Field>> &_lines;
        };
}

void ListForm::Controller::refresh()
{
	// Render our commands and entries down to some text lines.
	_lines.clear();
	LineBuilder fields(_lines);
	std::unique_ptr<Field> field(new Blank);
	fields.add(std::move(field));
	render(fields);
	field.reset(new Blank);
	fields.add(std::move(field));
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

void ListForm::Controller::paint_line(WINDOW *view, int y, int height, int width)
{
	size_t line = (size_t)y + _scrollpos;
	wmove(view, y, 0);
	if (line >= _lines.size()) {
		wclrtoeol(view);
		return;
	}
	auto &field = _lines[line];
	field->paint(view, (size_t)width);
	wclrtoeol(view);
	if (line == _selpos) {
		wmove(view, y, 0);
		field->highlight(view, (size_t)width);
	}
}

bool ListForm::Controller::is_selectable(ssize_t line)
{
	if (line < 0) return false;
	if ((size_t)line >= _lines.size()) return false;
	return _lines[line]->active();
}

void ListForm::Controller::arrow_down(WINDOW *view)
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

void ListForm::Controller::arrow_up(WINDOW *view)
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

void ListForm::Controller::commit(WINDOW *view)
{
	assert(_selpos < _lines.size());
	if (_lines[_selpos]->invoke()) {
		_dirty = true;
		paint(view);
	}
}

void ListForm::Controller::escape(WINDOW *view)
{
	assert(_selpos < _lines.size());
	if (_lines[_selpos]->cancel()) {
		_dirty = true;
		paint(view);
	}
}

void ListForm::Controller::scroll_to_selection(WINDOW *view)
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
