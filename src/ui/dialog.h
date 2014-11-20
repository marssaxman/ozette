//
// lindi
// Copyright (C) 2014 Mars J. Saxman
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
#include "helpbar.h"
#include "view.h"

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
	virtual void paint_into(WINDOW *view, bool active) override;
	virtual unsigned extra_height() const { return 0; }
private:
	std::string _prompt;
};


class Input : public Base
{
	typedef Base inherited;
public:
	typedef std::function<void(Frame&, std::string)> action_t;
	virtual bool process(UI::Frame &ctx, int ch) override;
	virtual bool poll(UI::Frame &ctx) override;
protected:
	Input(std::string prompt, action_t commit);
	virtual void paint_into(WINDOW *view, bool active) override;
	void select_suggestion(size_t i);
	void select_field();
	std::vector<std::string> _options;
	bool _suggestion_selected = false;
	size_t _sugg_item = 0;
	action_t _commit = nullptr;
	std::string _value;
	unsigned _cursor_pos = 0;
private:
	void arrow_left();
	void arrow_right();
	void delete_prev();
	void delete_next();
	void key_insert(int ch);
	void set_value(std::string val);
	bool _repaint = true;
};

// Yes/no confirmation dialog, with optional "all".
class Confirmation : public Base
{
	typedef Base inherited;
public:
	typedef std::function<void(UI::Frame &ctx)> action_t;
	Confirmation(std::string prompt, action_t yes, action_t no);
	virtual bool process(UI::Frame &ctx, int ch) override;
	virtual void set_help(HelpBar::Panel &panel) override;
private:
	action_t _yes;
	action_t _no;
	action_t _all = nullptr;
};

// Picker asks the user to enter a file path.
class Pick : public Input
{
	typedef Input inherited;
public:
	Pick(std::string prompt, std::vector<std::string> options, action_t commit);
	Pick(std::string prompt, std::string value, action_t commit);
	virtual bool process(Frame &ctx, int ch) override;
protected:
	virtual unsigned extra_height() const override { return _options.size(); }
	virtual void paint_into(WINDOW *view, bool active) override;
private:
	void arrow_left(Frame &ctx);
	void arrow_right(Frame &ctx);
	void arrow_up();
	void arrow_down();
};

class Find: public Input
{
public:
	Find(std::string prompt, action_t commit): Input(prompt, commit) {}
};

class GoLine: public Input
{
public:
	GoLine(std::string prompt, action_t commit): Input(prompt, commit) {}
};

class Command: public Input
{
public:
	Command(std::string prompt, action_t commit): Input(prompt, commit) {}
};

} // namespace Dialog
} // namespace UI

#endif UI_DIALOG_H
