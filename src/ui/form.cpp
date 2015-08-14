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
#include "ui/view.h"
#include "ui/helpbar.h"
#include <assert.h>

namespace {

// Single UI control managing a form field. This is a helper class for
// FormView, managing the state for a single form field.
class Input
{
public:
	Input(const UI::Form::Field &field);
	void process(UI::Frame &ctx, int ch);
	void paint(WINDOW *view, int row, UI::View::State state);
	void set_help(UI::HelpBar::Panel &panel);
	std::string name() const { return _field.name; }
	std::string value() const { return _field.value; }
	void set_indent(int i) { _indent = i; }
protected:
	void ctl_cut(UI::Frame &ctx);
	void ctl_copy(UI::Frame &ctx);
	void ctl_paste(UI::Frame &ctx);
	void arrow_left(UI::Frame &ctx);
	void arrow_right(UI::Frame &ctx);
	void select_left(UI::Frame &ctx);
	void select_right(UI::Frame &ctx);
	void delete_prev(UI::Frame &ctx);
	void delete_next(UI::Frame &ctx);
	void delete_selection(UI::Frame &ctx);
	void tab_complete(UI::Frame &ctx);
	void key_insert(UI::Frame &ctx, int ch);
	UI::Form::Field _field;
	int _indent = 0;
	unsigned _cursor_pos = 0;
	unsigned _anchor_pos = 0;
};

// View managing the active representation of the form
class FormView : public UI::View
{
	typedef UI::View inherited;
public:
	FormView(const UI::Form &form);
	virtual void layout(int vpos, int hpos, int height, int width) override;
	virtual bool process(UI::Frame &ctx, int ch) override;
	virtual void set_help(UI::HelpBar::Panel &panel) override;
protected:
	virtual void paint_into(WINDOW *view, State state) override;
	void paint_line(WINDOW *view, size_t i, State state);
	void key_up(UI::Frame &ctx);
	void key_down(UI::Frame &ctx);
	bool commit(UI::Frame &ctx, UI::Form::action_t action);
private:
	UI::Form _form;
	std::vector<Input> _inputs;
	size_t _selected = 0;
};

} // namespace

void UI::Form::show(Frame &ctx)
{
	std::unique_ptr<UI::View> dptr(new FormView(*this));
	ctx.show_dialog(std::move(dptr));
}

FormView::FormView(const UI::Form &form):
	inherited(),
	_form(form)
{
	size_t max_caption = 0;
	for (auto &field: _form.fields) {
		_inputs.emplace_back(field);
		max_caption = std::max(max_caption, field.name.size());
	}
	// Right-align the field names.
	for (auto &input: _inputs) {
		input.set_indent(max_caption - input.name().size());
	}
}

void FormView::layout(int vpos, int hpos, int height, int width)
{
	int new_height = std::min((int)_inputs.size(), height / 2);
	int new_vpos = vpos + height - new_height;
	inherited::layout(new_vpos, hpos, new_height, width);
}

bool FormView::process(UI::Frame &ctx, int ch)
{
	switch (ch) {
		case Control::Return:
		case Control::Enter: return commit(ctx, _form.commit);
		case KEY_UP: key_up(ctx); break;
		case KEY_DOWN: key_down(ctx); break;
		default: {
			assert(_selected < _inputs.size());
			_inputs[_selected].process(ctx, ch);
		} break;
	}
	return true;
}

void FormView::set_help(UI::HelpBar::Panel &panel)
{
	_inputs[_selected].set_help(panel);
}

void FormView::paint_into(WINDOW *view, State state)
{
	wattrset(view, UI::Colors::dialog(state == State::Focused));
	// Draw each field, below the title bar but above the help text.
	// Skip the selected field; we will draw it last, so it can set its
	// cursor / selection range graphics.
	State normal_state = state;
	if (normal_state == State::Focused) {
		normal_state = State::Active;
	}
	for (size_t i = 0; i < _inputs.size(); ++i) {
		if (i == _selected) continue;
		paint_line(view, i, normal_state);
	}
	paint_line(view, _selected, state);
}

void FormView::paint_line(WINDOW *view, size_t i, State state)
{
	int height, width;
	getmaxyx(view, height, width);
	(void)height;
	wmove(view, i, 0);
	whline(view, ' ', width);
	_inputs[i].paint(view, i, state);
}

void FormView::key_up(UI::Frame &ctx)
{
	if (_selected == 0) return;
	_selected--;
	ctx.repaint();
}

void FormView::key_down(UI::Frame &ctx)
{
	if (_selected + 1 >= _inputs.size()) return;
	_selected++;
	ctx.repaint();
}

bool FormView::commit(UI::Frame &ctx, UI::Form::action_t action)
{
	if (!action) return true;
	UI::Form::Result result;
	for (auto &input: _inputs) {
		result.fields[input.name()] = input.value();
	}
	result.selection = _selected;
	result.selected_value = _inputs[_selected].value();
	action(ctx, result);
	return false;
}

Input::Input(const UI::Form::Field &field):
	_field(field),
	_cursor_pos(field.value.size()),
	_anchor_pos(0)
{
}

void Input::process(UI::Frame &ctx, int ch)
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
}

void Input::paint(WINDOW *view, int row, UI::View::State state)
{
	int height, width;
	getmaxyx(view, height, width);
	(void)height;

	// Draw the caption, on the left side of the field.
	wmove(view, row, _indent);
	width -= _indent;
	waddnstr(view, _field.name.c_str(), width);
	width -= std::min(static_cast<int>(_field.name.size()), width);

	// Draw the current value, truncated to fit remaining space.
	int value_hpos, value_vpos;
	(void)value_vpos;
	if (width >= 2) {
		waddnstr(view, ": ", width);
		width -= 2;
		getyx(view, value_vpos, value_hpos);
		waddnstr(view, _field.value.c_str(), width);
		width -= std::min(static_cast<int>(_field.value.size()), width);
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

void Input::set_help(UI::HelpBar::Panel &panel)
{
	panel.cut();
	panel.copy();
	panel.paste();
}

void Input::ctl_cut(UI::Frame &ctx)
{
	ctl_copy(ctx);
	delete_selection(ctx);
}

void Input::ctl_copy(UI::Frame &ctx)
{
	// Don't replace the current clipboard contents unless something is
	// actually selected.
	if (_anchor_pos == _cursor_pos) {
		return;
	}
	unsigned begin = std::min(_anchor_pos, _cursor_pos);
	unsigned count = std::max(_anchor_pos - begin, _cursor_pos - begin);
	ctx.app().set_clipboard(_field.value.substr(begin, count));
}

void Input::ctl_paste(UI::Frame &ctx)
{
	delete_selection(ctx);
	std::string clip = ctx.app().get_clipboard();
	auto &value = _field.value;
	value = value.substr(0, _cursor_pos) + clip + value.substr(_cursor_pos);
	_cursor_pos += clip.size();
	_anchor_pos = _cursor_pos;
}

void Input::arrow_left(UI::Frame &ctx)
{
	if (_cursor_pos != _anchor_pos) {
		_cursor_pos = std::min(_cursor_pos, _anchor_pos);
		_anchor_pos = _cursor_pos;
		ctx.repaint();
	} else {
		select_left(ctx);
		_anchor_pos = _cursor_pos;
	}
}

void Input::arrow_right(UI::Frame &ctx)
{
	if (_cursor_pos != _anchor_pos) {
		_cursor_pos = std::max(_cursor_pos, _anchor_pos);
		_anchor_pos = _cursor_pos;
		ctx.repaint();
	} else {
		select_right(ctx);
		_anchor_pos = _cursor_pos;
	}
}

void Input::select_left(UI::Frame &ctx)
{
	if (_cursor_pos > 0) {
		_cursor_pos--;
		ctx.repaint();
	}
}

void Input::select_right(UI::Frame &ctx)
{
	if (_cursor_pos < _field.value.size()) {
		_cursor_pos++;
		ctx.repaint();
	}
}

void Input::delete_prev(UI::Frame &ctx)
{
	if (_cursor_pos == _anchor_pos) {
		select_left(ctx);
	}
	delete_selection(ctx);
}

void Input::delete_next(UI::Frame &ctx)
{
	if (_cursor_pos == _anchor_pos) {
		select_right(ctx);
	}
	delete_selection(ctx);
}

void Input::delete_selection(UI::Frame &ctx)
{
	if (_field.value.empty()) return;
	if (_cursor_pos == _anchor_pos) return;
	auto begin = std::min(_anchor_pos, _cursor_pos);
	auto end = std::max(_anchor_pos, _cursor_pos);
	auto &value = _field.value;
	value = value.substr(0, begin) + value.substr(end);
	_cursor_pos = begin;
	_anchor_pos = begin;
	ctx.repaint();
}

void Input::tab_complete(UI::Frame &ctx)
{
	// If we have an autocompleter function, give it a chance to extend the
	// text to the left of the insertion point.
	if (!_field.completer) return;
	size_t searchpos = std::min(_cursor_pos, _anchor_pos);
	std::string prefix = _field.value.substr(0, searchpos);
	std::string postfix = _field.value.substr(searchpos);
	std::string extended = _field.completer(prefix);
	if (extended == prefix) return;
	_field.value = extended + postfix;
	_cursor_pos = extended.size();
	_anchor_pos = extended.size();
	ctx.repaint();
}

void Input::key_insert(UI::Frame &ctx, int ch)
{
	delete_selection(ctx);
	_field.value.insert(_cursor_pos++, 1, ch);
	_anchor_pos = _cursor_pos;
	ctx.repaint();
}

