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

#include "ui/confirm.h"
#include "ui/colors.h"
#include <assert.h>

void UI::Confirm::show(
		UI::Frame &ctx, std::string text, action_t yes, action_t no)
{
	std::unique_ptr<View> dptr(new Confirm(text, yes, no));
	ctx.show_dialog(std::move(dptr));
}

UI::Confirm::Confirm(std::string text, action_t yes, action_t no):
	_text(text), _yes(yes), _no(no)
{
	assert(!text.empty());
	assert(_yes != nullptr);
	assert(_no != nullptr);
}

void UI::Confirm::layout(int vpos, int hpos, int height, int width)
{
	// A confirmation dialog always has exactly one line, with question text.
	inherited::layout(vpos + height - 1, hpos, 1, width);
}

bool UI::Confirm::process(UI::Frame &ctx, int ch)
{
	switch (ch) {
		case 'Y':
		case 'y': _yes(ctx); return false;
		case 'N':
		case 'n': _no(ctx); return false;
	}
	return true;
}

void UI::Confirm::set_help(UI::HelpBar::Panel &panel)
{
	panel.yes();
	panel.no();
}

void UI::Confirm::paint_into(WINDOW *view, State state)
{
	int height, width;
	getmaxyx(view, height, width);
	(void)height;
	wattrset(view, Colors::dialog(state == State::Focused));
	whline(view, ' ', width);
	mvwaddnstr(view, 0, 0, _text.c_str(), width);
}

