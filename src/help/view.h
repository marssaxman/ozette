// ozette
// Copyright (C) 2016 Mars J. Saxman
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

#ifndef HELP_VIEW_H
#define HELP_VIEW_H

#include "ui/shell.h"
#include "ui/view.h"

namespace Help {
class View : public UI::View {
public:
	static void show(UI::Shell &shell);
	virtual bool process(UI::Frame &ctx, int ch) override;
protected:
	View();
	~View();
	static View *_instance;
	UI::Window *_window = nullptr;
	virtual void paint_into(WINDOW *view, State state) override;
private:
	void key_up(UI::Frame &ctx);
	void key_down(UI::Frame &ctx);
	void key_page_up(UI::Frame &ctx);
	void key_page_down(UI::Frame &ctx);
	void scrollby(int delta, UI::Frame &ctx);
	unsigned _scrollpos = 0;
	int _height = 0;
	int _width = 0;
};
} // namespace Help

#endif // HELP_VIEW_H
