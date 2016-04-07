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

#ifndef DIALOG_INPUT_H
#define DIALOG_INPUT_H

#include "ui/view.h"
#include "ui/frame.h"
#include "ui/helpbar.h"

namespace Dialog {
// All common functionality for a single-line text input box.
class Input {
public:
	typedef std::function<std::string(std::string)> Completer;
	typedef std::function<void(UI::Frame &ctx)> Updater;
	Input(std::string value, Completer, Updater);
	void process(UI::Frame &ctx, int ch);
	void paint(WINDOW*, int vpos, int hpos, int width, UI::View::State);
	void set_help(UI::HelpBar::Panel &panel);
	std::string value() const { return _value; }
	void select_all(UI::Frame &ctx);
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
	std::string _value;
	Completer _completer;
	Updater _updater;
	unsigned _cursor_pos = 0;
	unsigned _anchor_pos = 0;
};
} // namespace Dialog

#endif //DIALOG_INPUT_H

