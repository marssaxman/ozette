#include "dialog.h"
#include "frame.h"
#include <assert.h>

UI::Dialog::Base::Base(std::string prompt):
	_prompt(prompt)
{
}

void UI::Dialog::Base::layout(int vpos, int hpos, int height, int width)
{
	int content_height = 1 + extra_height();
	int new_height = std::min(content_height, height / 2);
	int new_vpos = vpos + height - new_height;
	inherited::layout(new_vpos, hpos, new_height, width);
}

bool UI::Dialog::Base::process(UI::Frame &ctx, int ch)
{
	switch (ch) {
		case Control::Escape:	// escape key
		case Control::Close:	// control-W
			// the user no longer wants this action
			// this dialog has no further purpose
			ctx.show_result("Cancelled");
			return false;
		default: break;
	}
	return true;
}

void UI::Dialog::Base::paint_into(WINDOW *view, bool active)
{
	// Everything drawn in a dialog is reversed by default.
	wattron(view, A_REVERSE);
	// Fill the dialog window.
	wmove(view, 0, 0);
	int height, width;
	getmaxyx(view, height, width);
	(void)height; // unused
	whline(view, ' ', width);
	waddnstr(view, _prompt.c_str(), width);
	wattroff(view, A_REVERSE);
}

void UI::Dialog::Base::set_help(HelpBar::Panel &panel)
{
	panel.label[1][0] = HelpBar::Label('[', true, "Escape");
}

UI::Dialog::Input::Input(const Layout &layout):
	Base(layout.prompt),
	_layout(layout)
{
	if (_layout.value.empty() && !_layout.options.empty()) {
		_suggestion_selected = true;
		_sugg_item = 0;
		_layout.value = _layout.options.front();
	}
	_cursor_pos = _layout.value.size();
}

bool UI::Dialog::Input::process(UI::Frame &ctx, int ch)
{
	switch (ch) {
		case Control::Return:
		case Control::Enter:
			// the user is happy with their choice
			// tell the action to proceed and then
			// inform our host that we are finished
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
			// in all other situations, the keypress should be
			// inserted into the field at the cursor point.
			key_insert(ch);
			break;
	}
	if (_repaint) {
		ctx.repaint();
	}
	return inherited::process(ctx, ch);
}

void UI::Dialog::Input::paint_into(WINDOW *view, bool active)
{
	inherited::paint_into(view, active);

	wattron(view, A_REVERSE);
	waddstr(view, ": ");
	int height, width;
	getmaxyx(view, height, width);
	int value_vpos, value_hpos;
	getyx(view, value_vpos, value_hpos);
	(void)value_vpos; // unused
	waddnstr(view, _layout.value.c_str(), width - value_hpos);
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
	curs_set(show_cursor? 1: 0);
}

void UI::Dialog::Input::arrow_left()
{
	if (_suggestion_selected) {
		select_field();
		_cursor_pos = _layout.value.size();
	} else {
		_cursor_pos -= std::min(_cursor_pos, 1U);
		_repaint = true;
	}
}

void UI::Dialog::Input::arrow_right()
{
	if (_suggestion_selected) {
		select_field();
		_cursor_pos = 0;
	} else if (_cursor_pos < _layout.value.size()) {
		_cursor_pos++;
		_repaint = true;
	}
}

void UI::Dialog::Input::arrow_up()
{
	if (!_suggestion_selected) return;
	if (_sugg_item > 0) {
		select_suggestion(_sugg_item - 1);
	} else {
		select_field();
	}
}

void UI::Dialog::Input::arrow_down()
{
	if (_suggestion_selected) {
		select_suggestion(_sugg_item + 1);
	} else {
		select_suggestion(0);
	}
}

void UI::Dialog::Input::delete_prev()
{
	select_field();
	if (_layout.value.empty()) return;
	if (_cursor_pos == 0) return;
	_cursor_pos--;
	auto deliter = _layout.value.begin() + _cursor_pos;
	_layout.value.erase(deliter);
	_repaint = true;
}

void UI::Dialog::Input::delete_next()
{
	select_field();
	if (_layout.value.empty()) return;
	if (_cursor_pos >= _layout.value.size()) return;
	auto deliter = _layout.value.begin() + _cursor_pos;
	_layout.value.erase(deliter);
	_repaint = true;
}

void UI::Dialog::Input::key_insert(int ch)
{
	select_field();
	_layout.value.insert(_cursor_pos++, 1, ch);
	_repaint = true;
}

void UI::Dialog::Input::select_suggestion(size_t i)
{
	if (i >= _layout.options.size()) return;
	if (_suggestion_selected && _sugg_item == i) return;
	_suggestion_selected = true;
	_sugg_item = i;
	_repaint = true;
	set_value(_layout.options[i]);
}

void UI::Dialog::Input::select_field()
{
	if (!_suggestion_selected) return;
	_suggestion_selected = false;
	_cursor_pos = _layout.value.size();
	_repaint = true;
}

void UI::Dialog::Input::set_value(std::string val)
{
	if (val == _layout.value) return;
	_layout.value = val;
	_repaint = true;
}

UI::Dialog::Branch::Branch(std::string p, const std::vector<Option> &opts):
	Base(p),
	_options(opts)
{
}

bool UI::Dialog::Branch::process(UI::Frame &ctx, int ch)
{
	for (auto &opt: _options) {
		if (toupper(ch) == toupper(opt.key)) {
			opt.action(ctx);
			return false;
		}
	}
	return inherited::process(ctx, ch);
}

void UI::Dialog::Branch::set_help(HelpBar::Panel &panel)
{
	inherited::set_help(panel);
	unsigned v = 0;
	unsigned h = 0;
	for (auto &opt: _options) {
		if (h >= HelpBar::Panel::kWidth) {
			h = 1; // skip the cancel command
			v++;
		}
		if (v >= HelpBar::Panel::kHeight) {
			break;
		}
		panel.label[v][h].mnemonic = opt.key;
		panel.label[v][h].is_ctrl = false;
		panel.label[v][h].text = opt.description;
		h++;
	}
}
