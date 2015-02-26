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

#ifndef UI_VIEW_H
#define UI_VIEW_H

#include <ncurses.h>
#include <panel.h>
#include "frame.h"
#include "helpbar.h"

namespace UI {
class View
{
public:
	View();
	virtual ~View();
	virtual void layout(int v, int h, int height, int width);
	void bring_forward();
	enum class State {
		Inactive,
		Active,
		Focused
	};
	virtual void paint(State state);
	void overlay_result(std::string result, State state);
	virtual void clear_overlay() {}
	virtual void activate(Frame &ctx) {}
	virtual void deactivate(Frame &ctx) {}
	virtual bool process(Frame &ctx, int ch) = 0;
	virtual bool poll(Frame &ctx) { return true; }
	virtual void set_help(HelpBar::Panel &panel) {}
	enum class Priority {
		Primary,
		Secondary,
		Any
	};
	virtual Priority priority() const { return Priority::Any; }
protected:
	virtual void paint_into(WINDOW *view, State state) = 0;
private:
	WINDOW *_window = nullptr;
	PANEL *_panel = nullptr;
};
} // namespace UI

#endif // UI_VIEW_H
