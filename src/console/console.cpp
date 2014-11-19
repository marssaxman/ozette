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

#include "console.h"
#include "control.h"
#include "dialog.h"
#include <cctype>

Console *Console::_instance;

void Console::exec(std::string cmd, UI::Shell &shell)
{
	if (_instance) {
		shell.make_active(_instance->_window);
	} else {
		std::unique_ptr<UI::View> view(new Console());
		_instance->_window = shell.open_window(std::move(view));
	}
}

void Console::activate(UI::Frame &ctx)
{
}

void Console::deactivate(UI::Frame &ctx)
{
}

void Console::paint_into(WINDOW *view, bool active)
{
}

bool Console::process(UI::Frame &ctx, int ch)
{
	switch (ch) {
		case Control::Close: return false;
	}
	return true;
}

bool Console::poll(UI::Frame &ctx)
{
	return true;
}
