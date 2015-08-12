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

#include "ui/view.h"

// A dialog box asks the user a question, then waits for their answer.
// They may answer "yes" or "no", or they may cancel the operation that
// prompted the question.
namespace UI {
class Dialog : public UI::View {
	typedef UI::View inherited;
public:
	typedef std::function<void(Frame&)> action_t;
	static void show(Frame &ctx, std::string text, action_t yes, action_t no);
	Dialog(std::string text, action_t yes, action_t no);

	virtual void layout(int vpos, int hpos, int height, int width) override;
	virtual bool process(UI::Frame &ctx, int ch) override;
	virtual void set_help(HelpBar::Panel &panel) override;
protected:
	virtual void paint_into(WINDOW *view, State state) override;
	std::string _text;
	action_t _yes = nullptr;
	action_t _no = nullptr;
};
} // namespace UI

#endif UI_DIALOG_H
