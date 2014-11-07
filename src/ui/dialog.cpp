#include "dialog.h"

UI::Dialog::Dialog(std::unique_ptr<Action> &&action):
	_win(newwin(0, 0, 0, 0)),
	_panel(new_panel(_win)),
	_action(std::move(action))
{
	_action->open(_state);
	if (_state.suggestions.empty()) {
	} else {
		_suggestion_selected = true;
		_sugg_item = 0;
		_state.value = _state.suggestions[0];
	}
	_cursor_pos = _state.value.size();
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
	update_window_dimensions();
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
			_action->commit(ctx, _state.value);
			return false;
		case Control::Tab: tab_autofill(); break;
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
				_action->commit(ctx, _state.value);
				return false;
			}
			// in all other situations, the keypress should be
			// inserted into the field at the cursor point.
			key_insert(ch);
			break;
	}
	if (_update) {
		update_window_dimensions();
	}
	if (_repaint) {
		paint();
	}
	return true;
}

void UI::Dialog::paint()
{
	// Everything drawn in a dialog is reversed by default.
	wattron(_win, A_REVERSE);

	int height, width;
	getmaxyx(_win, height, width);
	(void)height; // unused

	// Draw the prompt and the current value string.
	wmove(_win, 0, 0);
	if (!_state.prompt.empty()) {
		waddnstr(_win, _state.prompt.c_str(), width);
		waddch(_win, ':');
		waddch(_win, ' ');
	}
	int value_vpos, value_hpos;
	getyx(_win, value_vpos, value_hpos);
	(void)value_vpos; // unused
	if (!_suggestion_selected) wattron(_win, A_UNDERLINE);
	waddnstr(_win, _state.value.c_str(), width - value_hpos);
	if (!_suggestion_selected) wattroff(_win, A_UNDERLINE);
	int end_vpos, end_hpos;
	getyx(_win, end_vpos, end_hpos);
	(void)end_vpos; // unused
	whline(_win, ' ', width - end_hpos);

	if (!_state.suggestions.empty()) {
		// Draw each suggested value on its own line.
		int sugg_vpos = value_vpos + 1;
		int sugg_width = width - 4;
		for (unsigned i = 0; i < _state.suggestions.size(); ++i) {
			wmove(_win, sugg_vpos + i, 0);
			if (i < 10 && _suggestion_selected) {
				waddch(_win, '0' + i);
				waddch(_win, ':');
			} else {
				waddstr(_win, "  ");
			}
			waddnstr(_win, _state.suggestions[i].c_str(), sugg_width);
			int curv, curh;
			getyx(_win, curv, curh);
			(void)curv; //ignored
			whline(_win, ' ', width - curh);
		}
		// Draw a blank line underneath.
		wmove(_win, sugg_vpos + _state.suggestions.size(), 0);
		whline(_win, ' ', width);
		// If one of these items was selected, highlight it by using
		// normal (non-reversed) mode.
		if (_suggestion_selected) {
			int selrow = sugg_vpos + _sugg_item;
			wmove(_win, selrow, 2);
			wchgat(_win, sugg_width, A_NORMAL, 0, NULL);
		}
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

void UI::Dialog::update_window_dimensions()
{
	// We will put the dialog at the bottom of its window, as wide as the
	// host and as many rows tall as its content, up to half the height of
	// the host window.
	int content_height = 1;
	if (!_state.suggestions.empty()) {
		// Draw each suggested value on its own row
		content_height += _state.suggestions.size();
		// Put a blank line on the bottom to separate the dialog
		// from whatever might be hanging out below it
		content_height++;
	}
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
	_update = false;
	_repaint = true;
}

void UI::Dialog::tab_autofill()
{
	// user wants some help filling this form out
	// ask the action object for its advice
	_action->autofill(_state);
	select_field();
	// we have no idea what the action object might have
	// changed, so we need to check everything for updates
	_update = true;
}

void UI::Dialog::arrow_left()
{
	if (_suggestion_selected) {
		select_field();
		_cursor_pos = _state.value.size();
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
	} else if (_cursor_pos < _state.value.size()) {
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
	if (_state.value.empty()) return;
	if (_cursor_pos == 0) return;
	_cursor_pos--;
	auto deliter = _state.value.begin() + _cursor_pos;
	_state.value.erase(deliter);
	update_action();
}

void UI::Dialog::delete_next()
{
	select_field();
	if (_state.value.empty()) return;
	if (_cursor_pos >= _state.value.size()) return;
	auto deliter = _state.value.begin() + _cursor_pos;
	_state.value.erase(deliter);
	update_action();
}

void UI::Dialog::key_insert(int ch)
{
	select_field();
	_state.value.insert(_cursor_pos++, 1, ch);
	update_action();
}

void UI::Dialog::select_suggestion(size_t i)
{
	if (i >= _state.suggestions.size()) return;
	if (_suggestion_selected && _sugg_item == i) return;
	_suggestion_selected = true;
	_sugg_item = i;
	_repaint = true;
	set_value(_state.suggestions[i]);
}

void UI::Dialog::select_field()
{
	if (!_suggestion_selected) return;
	_suggestion_selected = false;
	_cursor_pos = _state.value.size();
	_repaint = true;
}

void UI::Dialog::set_value(std::string val)
{
	if (val == _state.value) return;
	_state.value = val;
	update_action();
}

void UI::Dialog::update_action()
{
	_action->update(_state);
	// we don't know what the action might have done,
	// so we'll assume it did everything.
	_update = true;
}
