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
	Input(const Layout &layout);
	virtual bool process(UI::Frame &ctx, int ch) override;
protected:
	virtual void paint_into(WINDOW *view, bool active) override;
	virtual unsigned extra_height() const override { return _layout.options.size(); }
private:
	void arrow_left();
	void arrow_right();
	void arrow_up();
	void arrow_down();
	void delete_prev();
	void delete_next();
	void key_insert(int ch);
	void select_suggestion(size_t i);
	void select_field();
	void set_value(std::string val);

	// Layout structure supplied by the client.
	Layout _layout;

	// The cursor may be in the edit field or the suggestion list.
	size_t _cursor_pos = 0;
	bool _suggestion_selected = false;
	size_t _sugg_item = 0;
	// Do we need to repaint the window?
	bool _repaint = true;

};

// Present a branch dialog when the job meets a fork in the road and needs
// the user to select the appropriate direction.
class Branch : public Base
{
	typedef Base inherited;
public:
	struct Option {
		char key = '\0';
		std::string description;
		std::function<void(UI::Frame &ctx)> action = nullptr;
	};
	Branch(std::string prompt, const std::vector<Option> &options);
	virtual bool process(UI::Frame &ctx, int ch) override;
	virtual void set_help(HelpBar::Panel &panel) override;
private:
	std::vector<Option> _options;
};

} // namespace Dialog
} // namespace UI

#endif UI_DIALOG_H
