#include "dialog.h"

UI::Dialog::Dialog(std::unique_ptr<Controller> &&action):
	_win(newwin(0, 0, 0, 0)),
	_panel(new_panel(_win)),
	_action(std::move(action))
{
	_value = _action->open(_state);
	_cursor_pos = _value.size();
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
	paint();
}

void UI::Dialog::set_focus()
{
	_has_focus = true;
	paint();
}

void UI::Dialog::clear_focus()
{
	_has_focus = false;
	paint();
}

void UI::Dialog::bring_forward()
{
	top_panel(_panel);
}

bool UI::Dialog::process(int ch)
{
	bool needs_update = false;
	bool needs_paint = false;
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
			_action->commit(_value);
			return false;
		case Control::Tab: {
			// user wants some help filling this out;
			// state potentially changed so we must
			// perform a full update
			_value = _action->autofill(_value, _state);
			_cursor_pos = _value.size();
			needs_update = true;
		} break;
		case KEY_LEFT: {
			_cursor_pos -= std::min(_cursor_pos, 1U);
			needs_paint = true;
		} break;
		case KEY_RIGHT: {
			size_t curlimit = _value.size();
			_cursor_pos = std::min(_cursor_pos+1, curlimit);
			needs_paint = true;
		} break;
		case KEY_UP: {
			if (!_suggestion_selected) break;
			if (_sugg_item > 0) {
				_sugg_item--;
				_value = _state.suggestions[_sugg_item];
			} else {
				_suggestion_selected = false;
			}
			_cursor_pos = _value.size();
			needs_paint = true;
		} break;
		case KEY_DOWN: {
			if (!_suggestion_selected) {
				_suggestion_selected = true;
				_sugg_item = 0;
			} else if (_sugg_item < _state.suggestions.size()){
				_sugg_item++;
			}
			_value = _state.suggestions[_sugg_item];
			_cursor_pos = _value.size();
			needs_paint = true;
		} break;
		case Control::Backspace: {
			if (_value.empty()) break;
			if (_cursor_pos == 0) break;
			_cursor_pos--;
			auto deliter = _value.begin() + _cursor_pos;
			_value.erase(deliter);
			_action->update(_value, _state);
			needs_update = true;
		} break;
		case Control::Delete: {
			if (_value.empty()) break;
			if (_cursor_pos >= _value.size()) break;
			auto deliter = _value.begin() + _cursor_pos;
			_value.erase(deliter);
			_action->update(_value, _state);
			needs_update = true;
		} break;
		default:
			// if this is a non-control character,
			// add it to the string at the current
			// cursor position
			if (ch >= 32 && ch < 127) {
				_value.insert(_cursor_pos++, 1, ch);
				_action->update(_value, _state);
				needs_update = true;
			}
	}
	if (needs_update) {
		update_window_dimensions();
		needs_paint = true;
	}
	if (needs_paint) {
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
	waddnstr(_win, _state.prompt.c_str(), width);
	waddch(_win, ':');
	waddch(_win, ' ');
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

	if (!_state.suggestions.empty()) {
		// Draw each suggested value on its own line.
		int sugg_vpos = value_vpos + 1;
		int sugg_width = width - 4;
		for (unsigned i = 0; i < _state.suggestions.size(); ++i) {
			wmove(_win, sugg_vpos + i, 0);
			waddch(_win, ' ');
			waddch(_win, ' ');
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
	curs_set(_has_focus && !_suggestion_selected ? 1 : 0);
}

