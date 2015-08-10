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

#ifndef UI_FORM_H
#define UI_FORM_H

#include <vector>
#include <memory>
#include <functional>
#include "ui/view.h"

namespace UI {
// A view presenting a title and managing an array of fields, multiplexing
// user input among them; can be committed (enter) or cancelled (escape).
class Form : public UI::View
{
	typedef UI::View inherited;
public:
	// Abstract field representing one row of input within a form
	class Field
	{
	public:
		virtual bool process(UI::Frame &ctx, int ch) = 0;
		virtual void paint(WINDOW *view, int row, UI::View::State state) = 0;
		virtual void set_help(HelpBar::Panel &panel) = 0;
	};
	typedef std::vector<std::unique_ptr<Field>> FieldList;
	typedef std::function<void(Frame&)> action_t;

	// convenience wrappers for instantiating & showing a form
	static void show(
			UI::Frame &ctx,
			std::unique_ptr<Field> &&field,
			action_t action = nullptr);
	static void show(
			UI::Frame &ctx,
			FieldList &&fields,
			action_t action = nullptr);
	Form(FieldList &&fields, action_t action);

	virtual void layout(int vpos, int hpos, int height, int width) override;
	virtual bool process(UI::Frame &ctx, int ch) override;
	virtual void set_help(HelpBar::Panel &panel) override;
protected:
	virtual void paint_into(WINDOW *view, State state) override;
	void paint_line(WINDOW *view, size_t i, State state);
	void key_up(UI::Frame &ctx);
	void key_down(UI::Frame &ctx);
private:
	FieldList _fields;
	size_t _selected = 0;
	action_t _commit = nullptr;
};

// A static message, with optional yes/no decision actions.
class Label : public Form::Field
{
public:
	Label(std::string text): _text(text) {}
	Label(std::string text, Form::action_t yes, Form::action_t no):
		_text(text), _yes(yes), _no(no) {}
	virtual bool process(UI::Frame &ctx, int ch) override;
	virtual void paint(WINDOW *view, int row, UI::View::State state) override;
	virtual void set_help(HelpBar::Panel &panel) override;
private:
	std::string _text;
	Form::action_t _yes = nullptr;
	Form::action_t _no = nullptr;
};

// A form field in which the user can enter text.
class Input : public Form::Field
{
public:
	typedef std::function<std::string(std::string)> Completer;
	Input(std::string caption, std::string value, Completer completer=nullptr);
	virtual bool process(UI::Frame &ctx, int ch) override;
	virtual void paint(WINDOW *view, int row, UI::View::State state) override;
	virtual void set_help(HelpBar::Panel &panel) override;
	virtual std::string value() const { return _value; }
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
	std::string _caption;
	std::string _value;
	unsigned _cursor_pos = 0;
	unsigned _anchor_pos = 0;
	Completer _completer = nullptr;
};

} // namespace UI

#endif //UI_FORM_H

