// ozette
// Copyright (C) 2014-2016 Mars J. Saxman
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

#include "ui/colors.h"
#include <ncurses.h>

void UI::Colors::init() {
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
	init_pair(1, COLOR_GREEN, -1);
	init_pair(2, COLOR_YELLOW, -1);
	init_pair(3, COLOR_CYAN, -1);
	init_pair(4, COLOR_RED, -1);
	init_pair(5, COLOR_MAGENTA, -1);
}

int UI::Colors::content(bool active) { return active? A_NORMAL: A_DIM; }
int UI::Colors::chrome(bool active) { return active? A_NORMAL: A_DIM; }
int UI::Colors::dialog(bool active) { return active? A_REVERSE: A_DIM; }
int UI::Colors::result(bool active) { return active? A_REVERSE: A_DIM; }

int UI::Colors::keyword() { return A_BOLD | COLOR_PAIR(1); }
int UI::Colors::string() { return A_BOLD | COLOR_PAIR(2); }
int UI::Colors::literal() { return A_BOLD | COLOR_PAIR(3); }
int UI::Colors::comment() { return A_NORMAL | COLOR_PAIR(4); }
int UI::Colors::error() { return A_REVERSE | COLOR_PAIR(5); }

