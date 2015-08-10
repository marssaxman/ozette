//
// ozette
// Copyright (C) 2014-2015 Mars J. Saxman
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

#ifndef UI_DIALOG_H
#define UI_DIALOG_H

#include <string>
#include <functional>
#include <memory>
#include <ncurses.h>
#include <panel.h>
#include <vector>
#include "ui/helpbar.h"
#include "ui/view.h"

// A dialog is a modal input control.
namespace UI {
namespace Dialog {
class Base : public UI::View
{
	typedef UI::View inherited;
public:
	Base(std::string prompt);
	virtual void layout(int vpos, int hpos, int height, int width) override;
	virtual bool process(UI::Frame &ctx, int ch) override;
	virtual void set_help(HelpBar::Panel &panel) override;
protected:
	virtual void paint_into(WINDOW *view, State state) override;
	virtual int content_height() const { return 1; }
private:
	std::string _prompt;
};


class Input : public Base
{
	typedef Base inherited;
public:
	typedef std::function<void(Frame&, std::string)> action_t;
	Input(std::string prompt, action_t commit);
	virtual bool process(UI::Frame &ctx, int ch) override;
	virtual bool poll(UI::Frame &ctx) override;
protected:
	virtual void paint_into(WINDOW *view, State state) override;
	virtual void select_field();
	void set_value(std::string val);
	void repaint() { _repaint = true; }
	virtual bool field_selected() const { return true; }
	void move_cursor(unsigned pos);
	unsigned cursor_pos() const { return _cursor_pos; }
	action_t _commit = nullptr;
	std::string _value;
private:
	void ctl_cut(UI::Frame &ctx);
	void ctl_copy(UI::Frame &ctx);
	void ctl_paste(UI::Frame &ctx);
	void arrow_left();
	void arrow_right();
	void select_left();
	void select_right();
	void delete_prev();
	void delete_next();
	void delete_selection();
	void key_insert(int ch);
	unsigned _anchor_pos = 0;
	unsigned _cursor_pos = 0;
	bool _repaint = true;
};

} // namespace Dialog
} // namespace UI

#endif UI_DIALOG_H
