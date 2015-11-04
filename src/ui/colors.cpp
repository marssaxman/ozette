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

#include "ui/colors.h"
#include <ncurses.h>

namespace {
// these are attribute values, not color pair indexes
int sContent = A_NORMAL;
int sChrome = A_NORMAL;
int sDialog = A_NORMAL;
int sInactive = A_DIM;

int sIdentifier = A_NORMAL;
int sKeyword = A_DIM;
int sString = A_BOLD;
int sComment = A_NORMAL;
int sTrailingSpace = A_REVERSE;
} // namespace

int UI::Colors::content(bool active)
{
	return active? sContent: sInactive;
}

int UI::Colors::chrome(bool active)
{
	return active? sChrome: sInactive;
}

int UI::Colors::dialog(bool active)
{
	return active? (A_REVERSE|sDialog): sInactive;
}

int UI::Colors::result(bool active)
{
	return active? (A_REVERSE|sChrome): sInactive;
}

int UI::Colors::identifier() { return sIdentifier; }
int UI::Colors::keyword() { return sKeyword; }
int UI::Colors::string() { return sString; }
int UI::Colors::comment() { return sComment; }
int UI::Colors::trailing_space() { return sTrailingSpace; }

void UI::Colors::init()
{
	// Tell ncurses we want to use color if the terminal supports it.
	start_color();
	// If the terminal does not support color, there's no point in doing any
	// of this other stuff.
	if (!has_colors()) return;
	// There does not appear to be any way to find out what the default colors
	// are, so we have no way to avoid reusing the default foreground color.
	// I guess we just have to specify colors uniquely for this app, instead of
	// trying to adapt to the user's terminal settings, which seems... bad.
	use_default_colors();
	init_pair(1, COLOR_WHITE, -1);
	init_pair(2, COLOR_YELLOW, -1);
	init_pair(3, COLOR_CYAN, -1);

	sContent = COLOR_PAIR(0);
	sChrome = COLOR_PAIR(0);
	sDialog = COLOR_PAIR(0);
	sInactive = COLOR_PAIR(1);

	sKeyword |= COLOR_PAIR(2);
	sString |= COLOR_PAIR(2);
	sComment = COLOR_PAIR(3);
	sTrailingSpace |= COLOR_PAIR(3);
}

