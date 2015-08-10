//
// ozette
// Copyright (C) 2015 Mars J. Saxman
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//

#include "ui/form.h"
#include "ui/colors.h"

void UI::Form::show(
		UI::Frame &ctx, std::unique_ptr<Field> &&field, action_t action)
{
	FieldList fields;
	fields.emplace_back(std::move(field));
	show(ctx, std::move(fields), action);
}

void UI::Form::show(UI::Frame &ctx, FieldList &&fields, action_t action)
{
	std::unique_ptr<UI::View> dptr(new Form(std::move(fields), action));
	ctx.show_dialog(std::move(dptr));
}

UI::Form::Form(FieldList &&fields, action_t commit):
	_fields(std::move(fields)),
	_commit(commit)
{
}

void UI::Form::layout(int vpos, int hpos, int height, int width)
{
	int new_height = std::min((int)_fields.size(), height / 2);
	int new_vpos = vpos + height - new_height;
	inherited::layout(new_vpos, hpos, new_height, width);
}

bool UI::Form::process(UI::Frame &ctx, int ch)
{
	switch (ch) {
		case Control::Return:
		case Control::Enter:
			// the user is happy with their choice, so tell the action to
			// proceed and then inform our host that we are finished
			if (_commit) _commit(ctx);
			return false;
		case Control::Escape:	// escape key
			// the user no longer wants this action
			// this dialog has no further purpose
			ctx.show_result("Cancelled");
			return false;
		case KEY_UP: key_up(ctx); break;
		case KEY_DOWN: key_down(ctx); break;
		default: {
			if (_selected < _fields.size()) {
				return _fields[_selected]->process(ctx, ch);
			}
		} break;
	}
	return true;
}

void UI::Form::set_help(HelpBar::Panel &panel)
{
	if (_selected < _fields.size()) {
		_fields[_selected]->set_help(panel);
	}
	panel.label[1][0] = HelpBar::Label('[', true, "Escape");
}

void UI::Form::paint_into(WINDOW *view, State state)
{
	wattrset(view, Colors::dialog(state == State::Focused));
	// Draw each field, below the title bar but above the help text.
	// Skip the selected field; we will draw it last, so it can set its
	// cursor / selection range graphics.
	State normal_state = state;
	if (normal_state == State::Focused) {
		normal_state = State::Active;
	}
	for (size_t i = 0; i < _fields.size(); ++i) {
		if (i == _selected) continue;
		paint_line(view, i, normal_state);
	}
	paint_line(view, _selected, state);
}

void UI::Form::paint_line(WINDOW *view, size_t i, State state)
{
	int height, width;
	getmaxyx(view, height, width);
	(void)height;
	wmove(view, i, 0);
	whline(view, ' ', width);
	_fields[i]->paint(view, i, state);
}

void UI::Form::key_up(UI::Frame &ctx)
{
	if (_selected == 0) return;
	_selected--;
	ctx.repaint();
}

void UI::Form::key_down(UI::Frame &ctx)
{
	if (_selected + 1 >= _fields.size()) return;
	_selected++;
	ctx.repaint();
}

bool UI::Label::process(UI::Frame &ctx, int ch)
{
	switch (toupper(ch)) {
		case 'Y': return _yes? (_yes(ctx), false): true;
		case 'N': return _no? (_no(ctx), false): true;
		default: return true;
	}
}

void UI::Label::paint(WINDOW *view, int row, UI::View::State state)
{
	int height, width;
	getmaxyx(view, height, width);
	(void)height;
	mvwaddnstr(view, row, 0, _text.c_str(), width);
}

void UI::Label::set_help(HelpBar::Panel &panel)
{
	if (_yes) panel.label[0][0] = HelpBar::Label('Y', false, "Yes");
	if (_no) panel.label[0][1] = HelpBar::Label('N', false, "No");
}

UI::Input::Input(std::string caption, std::string value, Completer completer):
	_caption(caption),
	_value(value),
	_cursor_pos(value.size()),
	_anchor_pos(_cursor_pos),
	_completer(completer)
{
}

bool UI::Input::process(UI::Frame &ctx, int ch)
{
	switch (ch) {
		case Control::Cut: ctl_cut(ctx); break;
		case Control::Copy: ctl_copy(ctx); break;
		case Control::Paste: ctl_paste(ctx); break;
		case KEY_LEFT: arrow_left(ctx); break;
		case KEY_RIGHT: arrow_right(ctx); break;
		case KEY_SLEFT: select_left(ctx); break;
		case KEY_SRIGHT: select_right(ctx); break;
		case Control::Backspace: delete_prev(ctx); break;
		case KEY_DC: delete_next(ctx); break;
		case Control::Tab: tab_complete(ctx); break;
		default:
			// we only care about non-control chars now
			if (ch < 32 || ch > 127) break;
			// in all other situations, the keypress should be
			// inserted into the field at the cursor point.
			key_insert(ctx, ch);
			break;
	}
	return true;
}

void UI::Input::paint(WINDOW *view, int row, UI::View::State state)
{
	int height, width;
	getmaxyx(view, height, width);
	(void)height;

	// Draw the caption, on the left side of the field.
	wmove(view, row, 0);
	waddnstr(view, _caption.c_str(), width);
	width -= std::min(static_cast<int>(_caption.size()), width);

	// Draw the current value, truncated to fit remaining space.
	int value_hpos, value_vpos;
	(void)value_vpos;
	if (width >= 2) {
		waddnstr(view, ": ", width);
		width -= 2;
		getyx(view, value_vpos, value_hpos);
		waddnstr(view, _value.c_str(), width);
		width -= std::min(static_cast<int>(_value.size()), width);
	}

	// If there is space remaining, clear it out.
	if (width > 0) {
		int end_vpos, end_hpos;
		getyx(view, end_vpos, end_hpos);
		(void)end_vpos; // unused
		whline(view, ' ', width - end_hpos);
	}

	// Position the cursor, or draw the selection range.
	bool focused = (state == UI::View::State::Focused);
	if (_anchor_pos == _cursor_pos) {
		// Put the cursor where it ought to be. Make it visible, if that
		// would be appropriate for our activation state.
		wmove(view, row, value_hpos + _cursor_pos);
		curs_set(focused? 1: 0);
	} else {
		if (focused) {
			int selbegin = value_hpos + std::min(_cursor_pos, _anchor_pos);
			int selcount = std::abs((int)_cursor_pos - (int)_anchor_pos);
			mvwchgat(view, row, selbegin, selcount, A_NORMAL, 0, NULL);
		}
		curs_set(0);
	}
}

void UI::Input::set_help(HelpBar::Panel &panel)
{
	panel.label[0][0] = HelpBar::Label('X', true, "Cut");
	panel.label[0][1] = HelpBar::Label('C', true, "Copy");
	panel.label[0][2] = HelpBar::Label('V', true, "Paste");
}

void UI::Input::ctl_cut(UI::Frame &ctx)
{
	ctl_copy(ctx);
	delete_selection(ctx);
}

void UI::Input::ctl_copy(UI::Frame &ctx)
{
	// Don't replace the current clipboard contents unless something is
	// actually selected.
	if (_anchor_pos == _cursor_pos) {
		return;
	}
	unsigned begin = std::min(_anchor_pos, _cursor_pos);
	unsigned count = std::max(_anchor_pos - begin, _cursor_pos - begin);
	ctx.app().set_clipboard(_value.substr(begin, count));
}

void UI::Input::ctl_paste(UI::Frame &ctx)
{
	delete_selection(ctx);
	std::string clip = ctx.app().get_clipboard();
	_value = _value.substr(0, _cursor_pos) + clip + _value.substr(_cursor_pos);
	_cursor_pos += clip.size();
	_anchor_pos = _cursor_pos;
}

void UI::Input::arrow_left(UI::Frame &ctx)
{
	select_left(ctx);
	_anchor_pos = _cursor_pos;
}

void UI::Input::arrow_right(UI::Frame &ctx)
{
	select_right(ctx);
	_anchor_pos = _cursor_pos;
}

void UI::Input::select_left(UI::Frame &ctx)
{
	if (_cursor_pos > 0) {
		_cursor_pos--;
		ctx.repaint();
	}
}

void UI::Input::select_right(UI::Frame &ctx)
{
	if (_cursor_pos < _value.size()) {
		_cursor_pos++;
		ctx.repaint();
	}
}

void UI::Input::delete_prev(UI::Frame &ctx)
{
	if (_cursor_pos == _anchor_pos) {
		select_left(ctx);
	}
	delete_selection(ctx);
}

void UI::Input::delete_next(UI::Frame &ctx)
{
	if (_cursor_pos == _anchor_pos) {
		select_right(ctx);
	}
	delete_selection(ctx);
}

void UI::Input::delete_selection(UI::Frame &ctx)
{
	if (_value.empty()) return;
	if (_cursor_pos == _anchor_pos) return;
	auto begin = std::min(_anchor_pos, _cursor_pos);
	auto end = std::max(_anchor_pos, _cursor_pos);
	_value = _value.substr(0, begin) + _value.substr(end);
	_cursor_pos = begin;
	_anchor_pos = begin;
	ctx.repaint();
}

void UI::Input::tab_complete(UI::Frame &ctx)
{
	// If we have an autocompleter function, give it a chance to extend the
	// text to the left of the insertion point.
	if (!_completer) return;
	size_t searchpos = std::min(_cursor_pos, _anchor_pos);
	std::string prefix = _value.substr(0, searchpos);
	std::string postfix = _value.substr(searchpos);
	std::string extended = _completer(prefix);
	if (extended == prefix) return;
	_value = extended + postfix;
	_cursor_pos = extended.size();
	_anchor_pos = extended.size();
	ctx.repaint();
}

void UI::Input::key_insert(UI::Frame &ctx, int ch)
{
	delete_selection(ctx);
	_value.insert(_cursor_pos++, 1, ch);
	_anchor_pos = _cursor_pos;
	ctx.repaint();
}

