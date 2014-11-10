#include "dialog.h"
#include "frame.h"
#include <assert.h>

void UI::Dialog::Show(const Layout &layout, Frame &ctx)
{
	std::unique_ptr<Dialog> it(new Dialog(layout));
	ctx.show_dialog(std::move(it));
}

void UI::Dialog::layout(int vpos, int hpos, int height, int width)
{
	int content_height = 1 + _layout.options.size();
	int new_height = std::min(content_height, height / 2);
	int new_vpos = vpos + height - new_height;
	inherited::layout(new_vpos, hpos, new_height, width);
}

void UI::Dialog::set_help(HelpBar::Panel &panel)
{
	if (!_layout.show_value) {
		panel.label[0][0] = {'Y', false, "Yes"};
		panel.label[1][0] = {'N', false, "No"};
	}
	panel.label[1][5] = {'[', true, "Escape"};
}

bool UI::Dialog::process(UI::Frame &ctx, int ch)
{
	switch (ch) {
		case Control::Escape:	// escape key
		case Control::Close:	// control-W
			// the user no longer wants this action
			// this dialog has no further purpose
			ctx.show_result("Cancelled");
			return false;
		case Control::Return:
		case Control::Enter:
			// the user is happy with their choice
			// tell the action to proceed and then
			// inform our host that we are finished
			if(!_layout.show_value) break;
			if (_layout.commit) _layout.commit(ctx, _layout.value);
			return false;
		case KEY_LEFT: arrow_left(); break;
		case KEY_RIGHT: arrow_right(); break;
		case KEY_UP: arrow_up(); break;
		case KEY_DOWN: arrow_down(); break;
		case Control::Backspace: delete_prev(); break;
		case KEY_DC: delete_next(); break;
		default:
			// we only care about non-control chars now
			if (ch < 32 || ch > 127) break;
			// if this is a digit character and the selection
			// is on the suggestion list, insta-commit the
			// item corresponding to that digit
			if (ch >= '0' && ch <= '9' && _suggestion_selected) {
				select_suggestion(ch - '0');
				if (_layout.commit) _layout.commit(ctx, _layout.value);
				return false;
			}
			// if this is a non-value-showing dialog and the key
			// was "Y", that's commit; if it was "N", that's retry
			if (!_layout.show_value) {
				if (ch == 'Y' || ch == 'y') {
					if (_layout.yes) _layout.yes(ctx, _layout.value);
					return false;
				}
				if (ch == 'N' || ch == 'n') {
					if (_layout.no) _layout.no(ctx, _layout.value);
					return false;
				}
			}

			// in all other situations, the keypress should be
			// inserted into the field at the cursor point.
			key_insert(ch);
			break;
	}
	if (_repaint) {
		ctx.repaint();
	}
	return true;
}

UI::Dialog::Dialog(const Layout &layout):
	_layout(layout)
{
	if (_layout.value.empty() && !_layout.options.empty()) {
		_suggestion_selected = true;
		_sugg_item = 0;
		_layout.value = _layout.options.front();
	}
	_cursor_pos = _layout.value.size();
}

void UI::Dialog::paint(WINDOW *view, bool active)
{
	// Everything drawn in a dialog is reversed by default.
	wattron(view, A_REVERSE);

	int height, width;
	getmaxyx(view, height, width);

	// Draw the prompt and the current value string.
	wmove(view, 0, 0);
	waddnstr(view, _layout.prompt.c_str(), width);
	if (_layout.show_value) {
		waddstr(view, ": ");
	}
	int value_vpos, value_hpos;
	getyx(view, value_vpos, value_hpos);
	(void)value_vpos; // unused
	if (_layout.show_value) {
		if (!_suggestion_selected) wattron(view, A_UNDERLINE);
		waddnstr(view, _layout.value.c_str(), width - value_hpos);
		if (!_suggestion_selected) wattroff(view, A_UNDERLINE);
	}
	int end_vpos, end_hpos;
	getyx(view, end_vpos, end_hpos);
	(void)end_vpos; // unused
	whline(view, ' ', width - end_hpos);

	// Draw each suggested value on its own line.
	int sugg_vpos = value_vpos + 1;
	int sugg_width = width;
	int sugg_hpos = 0;
	// Reserve two columns on each side as a margin.
	sugg_width -= 4;
	sugg_hpos += 2;
	// Reduce the field width by three more chars to give
	// space for the quick-select number captions.
	sugg_width -= 3;

	for (unsigned i = 0; i < _layout.options.size(); ++i) {
		int vpos = sugg_vpos + i;
		if (vpos >= height) break;
		wmove(view, vpos, 0);
		if (i < 10 && _suggestion_selected) {
			waddstr(view, "  ");
			waddch(view, '0' + i);
			waddstr(view, ": ");
		} else {
			waddstr(view, "     ");
		}
		bool selrow = (_suggestion_selected && i == _sugg_item);
		if (selrow) {
			wattroff(view, A_REVERSE);
			if (vpos + 1 == height) wattron(view, A_UNDERLINE);
		}
		waddnstr(view, _layout.options[i].c_str(), sugg_width);
		int curv, curh;
		getyx(view, curv, curh);
		(void)curv; //ignored
		whline(view, ' ', width - curh - 2);
		if (selrow) {
			wattron(view, A_REVERSE);
			if (vpos + 1 == height) wattroff(view, A_UNDERLINE);
		}
		mvwaddstr(view, vpos, width - 2, "  ");
	}
	// We're done being all reversed and stuff.
	wattroff(view, A_REVERSE);

	// Put the cursor where it ought to be. Make it visible, if that
	// would be appropriate for our activation state.
	wmove(view, 0, value_hpos + _cursor_pos);
	bool show_cursor = active;
	show_cursor &= !_suggestion_selected;
	show_cursor &= _layout.show_value;
	curs_set(show_cursor? 1: 0);
}

void UI::Dialog::arrow_left()
{
	if (_suggestion_selected) {
		select_field();
		_cursor_pos = _layout.value.size();
	} else {
		_cursor_pos -= std::min(_cursor_pos, 1U);
		_repaint = true;
	}
}

void UI::Dialog::arrow_right()
{
	if (_suggestion_selected) {
		select_field();
		_cursor_pos = 0;
	} else if (_cursor_pos < _layout.value.size()) {
		_cursor_pos++;
		_repaint = true;
	}
}

void UI::Dialog::arrow_up()
{
	if (!_suggestion_selected) return;
	if (_sugg_item > 0) {
		select_suggestion(_sugg_item - 1);
	} else {
		select_field();
	}
}

void UI::Dialog::arrow_down()
{
	if (_suggestion_selected) {
		select_suggestion(_sugg_item + 1);
	} else {
		select_suggestion(0);
	}
}

void UI::Dialog::delete_prev()
{
	select_field();
	if (_layout.value.empty()) return;
	if (_cursor_pos == 0) return;
	_cursor_pos--;
	auto deliter = _layout.value.begin() + _cursor_pos;
	_layout.value.erase(deliter);
	_repaint = true;
}

void UI::Dialog::delete_next()
{
	select_field();
	if (_layout.value.empty()) return;
	if (_cursor_pos >= _layout.value.size()) return;
	auto deliter = _layout.value.begin() + _cursor_pos;
	_layout.value.erase(deliter);
	_repaint = true;
}

void UI::Dialog::key_insert(int ch)
{
	select_field();
	_layout.value.insert(_cursor_pos++, 1, ch);
	_repaint = true;
}

void UI::Dialog::select_suggestion(size_t i)
{
	if (i >= _layout.options.size()) return;
	if (_suggestion_selected && _sugg_item == i) return;
	_suggestion_selected = true;
	_sugg_item = i;
	_repaint = true;
	set_value(_layout.options[i]);
}

void UI::Dialog::select_field()
{
	if (!_suggestion_selected) return;
	_suggestion_selected = false;
	_cursor_pos = _layout.value.size();
	_repaint = true;
}

void UI::Dialog::set_value(std::string val)
{
	if (val == _layout.value) return;
	_layout.value = val;
	_repaint = true;
}
