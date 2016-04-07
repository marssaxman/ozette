// ozette
// Copyright (C) 2015-2016 Mars J. Saxman
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

#include "dialog/form.h"
#include "ui/colors.h"
#include "ui/view.h"
#include "ui/helpbar.h"
#include "dialog/input.h"
#include <assert.h>

namespace {
// Inputs for form fields fill the window width, but have a name caption
// on the left.
class FormInput {
public:
	FormInput(const Dialog::Form::Field &field);
	void process(UI::Frame &ctx, int ch);
	void paint(WINDOW *view, int row, UI::View::State state);
	void set_help(UI::HelpBar::Panel &panel);
	std::string name() const { return _field.name; }
	std::string value() const { return _input.value(); }
	void set_indent(int i);
private:
	Dialog::Form::Field _field;
	Dialog::Input _input;
	std::string _caption;
};

// View managing the active representation of the form
class FormView : public UI::View {
	typedef UI::View inherited;
public:
	FormView(const Dialog::Form &form);
	virtual void layout(int vpos, int hpos, int height, int width) override;
	virtual bool process(UI::Frame &ctx, int ch) override;
	virtual void set_help(UI::HelpBar::Panel &panel) override;
protected:
	virtual void paint_into(WINDOW *view, State state) override;
	void paint_line(WINDOW *view, size_t i, State state);
	void key_up(UI::Frame &ctx);
	void key_down(UI::Frame &ctx);
	bool commit(UI::Frame &ctx, Dialog::Form::action_t action);
private:
	Dialog::Form _form;
	std::vector<FormInput> _inputs;
	size_t _selected = 0;
};

} // namespace

void Dialog::Form::show(UI::Frame &ctx) {
	std::unique_ptr<UI::View> dptr(new FormView(*this));
	ctx.show_dialog(std::move(dptr));
}

FormView::FormView(const Dialog::Form &form): inherited(), _form(form) {
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

void FormView::layout(int vpos, int hpos, int height, int width) {
	int new_height = std::min((int)_inputs.size(), height / 2);
	int new_vpos = vpos + height - new_height;
	inherited::layout(new_vpos, hpos, new_height, width);
}

bool FormView::process(UI::Frame &ctx, int ch) {
	switch (ch) {
		case Control::Escape: ctx.show_result("Cancelled"); return false;
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

void FormView::set_help(UI::HelpBar::Panel &panel) {
	_inputs[_selected].set_help(panel);
}

void FormView::paint_into(WINDOW *view, State state) {
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

void FormView::paint_line(WINDOW *view, size_t i, State state) {
	int height, width;
	getmaxyx(view, height, width);
	(void)height;
	wmove(view, i, 0);
	whline(view, ' ', width);
	_inputs[i].paint(view, i, state);
}

void FormView::key_up(UI::Frame &ctx) {
	if (_selected == 0) return;
	_selected--;
	ctx.repaint();
}

void FormView::key_down(UI::Frame &ctx) {
	if (_selected + 1 >= _inputs.size()) return;
	_selected++;
	ctx.repaint();
}

bool FormView::commit(UI::Frame &ctx, Dialog::Form::action_t action) {
	if (!action) return true;
	Dialog::Form::Result result;
	for (auto &input: _inputs) {
		result.fields[input.name()] = input.value();
	}
	result.selection = _selected;
	result.selected_value = _inputs[_selected].value();
	action(ctx, result);
	return false;
}

FormInput::FormInput(const Dialog::Form::Field &field):
		_field(field), _input(field.value, field.completer, nullptr) {
	set_indent(0);
}

void FormInput::process(UI::Frame &ctx, int ch) {
	std::string old_value = _input.value();
	_input.process(ctx, ch);
	if (_field.updater && _input.value() != old_value) {
		_field.updater(_input.value());
	}
}

void FormInput::paint(WINDOW *view, int row, UI::View::State state) {
	int height, width;
	getmaxyx(view, height, width);
	(void)height;
	// Draw the caption, left-justified.
	mvwaddnstr(view, row, 0, _caption.c_str(), width);
	int column = std::min(static_cast<int>(_caption.size()), width);
	width -= column;
	// Tell the input to draw itself in the remaining space.
	_input.paint(view, row, column, width, state);
}

void FormInput::set_help(UI::HelpBar::Panel &panel) {
	_input.set_help(panel);
}

void FormInput::set_indent(int i) {
	std::string spacer(i, ' ');
	_caption = spacer + _field.name + ": ";
}

