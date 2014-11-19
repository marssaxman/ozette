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

#ifndef CONSOLE_CONSOLE_H
#define CONSOLE_CONSOLE_H

#include "view.h"
#include "shell.h"

class Console : public UI::View
{
public:
	static void exec(std::string cmd, UI::Shell &shell);
	virtual void activate(UI::Frame &ctx) override;
	virtual void deactivate(UI::Frame &ctx) override;
	virtual bool process(UI::Frame &ctx, int ch) override;
	virtual bool poll(UI::Frame &ctx) override;
protected:
	Console() { _instance = this; }
	~Console() { _instance = nullptr; }
	static Console *_instance;
	UI::Window *_window = nullptr;
	virtual void paint_into(WINDOW *view, bool active) override;
};

#endif // CONSOLE_CONSOLE_H
