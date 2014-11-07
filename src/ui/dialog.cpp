#include "dialog.h"
#include "frame.h"

void UI::Dialog::Show(const Input &options, Frame &ctx)
{
	std::unique_ptr<Dialog> it(new Dialog);
	it->_prompt = options.prompt;
	it->_value = options.value;
	it->_action = options.action;
	it->_cursor_pos = it->_value.size();
	ctx.show_dialog(std::move(it));
}

void UI::Dialog::Show(const Picker &options, Frame &ctx)
{
	std::unique_ptr<Dialog> it(new Dialog);
	it->_prompt = options.prompt;
	it->_suggestions = options.values;
	if (!it->_suggestions.empty()) {
		it->_suggestion_selected = true;
		it->_value = it->_suggestions.front();
	}
	it->_action = options.action;
	ctx.show_dialog(std::move(it));
}

UI::Dialog::~Dialog()
{
	del_panel(_panel);
	delwin(_win);
}

void UI::Dialog::layout(WINDOW *view)
{
	// The dialog should be sized for its content and positioned above the
	// specified window. Find out where the window is and how big it is so
	// we can compute appropriate dimensions for the dialog.
	getmaxyx(view, _host_height, _host_width);
	getbegyx(view, _host_v, _host_h);

	// Go lay the window out for these host dimensions and our current
	// content dimensions, which may change as the suggestion list changes.
	// We will put the dialog at the bottom of its window, as wide as the
	// host and as many rows tall as its content, up to half the height of
	// the host window.
	int content_height = 1 + _suggestions.size();
	int new_height = std::min(content_height, _host_height / 2);
	int new_width = _host_width;
	int new_v = _host_v + _host_height - new_height;
	int new_h = _host_h;

	// Find out where our window is located and how large it happens to be.
	// We may need to move and/or resize it.
	int old_height, old_width;
	getmaxyx(_win, old_height, old_width);
	int old_v, old_h;
	getbegyx(_win, old_v, old_h);
	if (new_height != old_height || new_width != old_width) {
		WINDOW *win = newwin(new_height, new_width, new_v, new_h);
		replace_panel(_panel, win);
		delwin(_win);
		_win = win;
	} else if (new_v != old_v || new_h != old_h) {
		move_panel(_panel, new_v, new_h);
	}
	paint();
}

void UI::Dialog::set_focus()
{
	if (!_has_focus) {
		_has_focus = true;
		paint();
	}
}

void UI::Dialog::clear_focus()
{
	if (_has_focus) {
		_has_focus = false;
		paint();
	}
}

void UI::Dialog::bring_forward()
{
	top_panel(_panel);
}

bool UI::Dialog::process(UI::Frame &ctx, int ch)
{
	switch (ch) {
		case Control::Escape:	// escape key
		case Control::Close:	// control-W
			// the user no longer wants this action
			// this dialog has no further purpose
			return false;
		case Control::Return:
		case Control::Enter:
			// the user is happy with their choice
			// tell the action to proceed and then
			// inform our host that we are finished
			_action(ctx, _value);
			return false;
		case KEY_LEFT: arrow_left(); break;
		case KEY_RIGHT: arrow_right(); break;
		case KEY_UP: arrow_up(); break;
		case KEY_DOWN: arrow_down(); break;
		case Control::Backspace: delete_prev(); break;
		case Control::Delete: delete_next(); break;
		default:
			// we only care about non-control chars now
			if (ch < 32 || ch > 127) break;
			// if this is a digit character and the selection
			// is on the suggestion list, insta-commit the
			// item corresponding to that digit
			if (ch >= '0' && ch <= '9' && _suggestion_selected) {
				select_suggestion(ch - '0');
				_action(ctx, _value);
				return false;
			}
			// in all other situations, the keypress should be
			// inserted into the field at the cursor point.
			key_insert(ch);
			break;
	}
	if (_repaint) {
		paint();
	}
	return true;
}

UI::Dialog::Dialog():
	_win(newwin(0, 0, 0, 0)),
	_panel(new_panel(_win))
{
}

void UI::Dialog::paint()
{
	// Everything drawn in a dialog is reversed by default.
	wattron(_win, A_REVERSE);

	int height, width;
	getmaxyx(_win, height, width);

	// Draw the prompt and the current value string.
	wmove(_win, 0, 0);
	if (!_prompt.empty()) {
		waddnstr(_win, _prompt.c_str(), width);
		waddch(_win, ':');
		waddch(_win, ' ');
	}
	int value_vpos, value_hpos;
	getyx(_win, value_vpos, value_hpos);
	(void)value_vpos; // unused
	if (!_suggestion_selected) wattron(_win, A_UNDERLINE);
	waddnstr(_win, _value.c_str(), width - value_hpos);
	if (!_suggestion_selected) wattroff(_win, A_UNDERLINE);
	int end_vpos, end_hpos;
	getyx(_win, end_vpos, end_hpos);
	(void)end_vpos; // unused
	whline(_win, ' ', width - end_hpos);

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

	for (unsigned i = 0; i < _suggestions.size(); ++i) {
		int vpos = sugg_vpos + i;
		if (vpos >= height) break;
		wmove(_win, vpos, 0);
		if (i < 10 && _suggestion_selected) {
			waddstr(_win, "  ");
			waddch(_win, '0' + i);
			waddstr(_win, ": ");
		} else {
			waddstr(_win, "     ");
		}
		bool selrow = (_suggestion_selected && i == _sugg_item);
		if (selrow) {
			wattroff(_win, A_REVERSE);
			if (vpos + 1 == height) wattron(_win, A_UNDERLINE);
		}
		waddnstr(_win, _suggestions[i].c_str(), sugg_width);
		int curv, curh;
		getyx(_win, curv, curh);
		(void)curv; //ignored
		whline(_win, ' ', width - curh - 2);
		if (selrow) {
			wattron(_win, A_REVERSE);
			if (vpos + 1 == height) wattroff(_win, A_UNDERLINE);
		}
		mvwaddstr(_win, vpos, width - 2, "  ");
	}
	// We're done being all reversed and stuff.
	wattroff(_win, A_REVERSE);

	// Put the cursor where it ought to be. Make it visible, if that
	// would be appropriate for our activation state.
	wmove(_win, 0, value_hpos + _cursor_pos);
	curs_set(_has_focus && !_suggestion_selected? 1: 0);

	// We no longer need to repaint.
	_repaint = false;
}

void UI::Dialog::arrow_left()
{
	if (_suggestion_selected) {
		select_field();
		_cursor_pos = _value.size();
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
	} else if (_cursor_pos < _value.size()) {
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
	if (_value.empty()) return;
	if (_cursor_pos == 0) return;
	_cursor_pos--;
	auto deliter = _value.begin() + _cursor_pos;
	_value.erase(deliter);
	_repaint = true;
}

void UI::Dialog::delete_next()
{
	select_field();
	if (_value.empty()) return;
	if (_cursor_pos >= _value.size()) return;
	auto deliter = _value.begin() + _cursor_pos;
	_value.erase(deliter);
	_repaint = true;
}

void UI::Dialog::key_insert(int ch)
{
	select_field();
	_value.insert(_cursor_pos++, 1, ch);
	_repaint = true;
}

void UI::Dialog::select_suggestion(size_t i)
{
	if (i >= _suggestions.size()) return;
	if (_suggestion_selected && _sugg_item == i) return;
	_suggestion_selected = true;
	_sugg_item = i;
	_repaint = true;
	set_value(_suggestions[i]);
}

void UI::Dialog::select_field()
{
	if (!_suggestion_selected) return;
	_suggestion_selected = false;
	_cursor_pos = _value.size();
	_repaint = true;
}

void UI::Dialog::set_value(std::string val)
{
	if (val == _value) return;
	_value = val;
	_repaint = true;
}
