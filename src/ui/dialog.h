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
	struct Layout {
		std::string prompt;
		std::string value;
		std::vector<std::string> options;
		action_t commit = nullptr;
	};
	virtual bool process(UI::Frame &ctx, int ch) override;
	virtual bool poll(UI::Frame &ctx) override;
protected:
	Input(const Layout &layout);
	virtual void paint_into(WINDOW *view, bool active) override;
	void select_suggestion(size_t i);
	void select_field();
	std::vector<std::string> _options;
	bool _suggestion_selected = false;
	size_t _sugg_item = 0;
	action_t _commit = nullptr;
	std::string _value;
	size_t _cursor_pos = 0;
private:
	void arrow_left();
	void arrow_right();
	void delete_prev();
	void delete_next();
	void key_insert(int ch);
	void set_value(std::string val);
	bool _repaint = true;
};

// Present a branch dialog when the job meets a fork in the road and needs
// the user to select the appropriate direction.
class Branch : public Base
{
	typedef Base inherited;
public:
	typedef std::function<void(UI::Frame &ctx)> action_t;
	struct Option {
		char key = '\0';
		std::string description;
		action_t action = nullptr;
	};
	Branch(std::string prompt, const std::vector<Option> &options);
	virtual bool process(UI::Frame &ctx, int ch) override;
	virtual void set_help(HelpBar::Panel &panel) override;
private:
	std::vector<Option> _options;
};

// Picker asks the user to enter a file path.
class Pick : public Input
{
	typedef Input inherited;
public:
	Pick(const Layout &layout);
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
	Find(const Layout &layout): Input(layout) {}
};

class GoLine: public Input
{
public:
	GoLine(const Layout &layout): Input(layout) {}
};

} // namespace Dialog
} // namespace UI

#endif UI_DIALOG_H
