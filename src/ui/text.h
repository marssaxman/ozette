// ozette
// Copyright (C) 2026 Mars J. Saxman
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

#ifndef UI_TEXT_H
#define UI_TEXT_H

#include <ncurses.h>
#include "text/layout.h"

namespace UI {
// Draw complete characters and tab fragments within a single row. Wide
// characters cut by an edge leave blank cells; combining marks stay with a base.
void paint_text(WINDOW *dest, int v, int h, int width,
	const Text::LineLayout &layout, unsigned scroll, int normal,
	const std::vector<int> &styles = {}, Text::LineLayout::Span selection = {});
} // namespace UI

#endif // UI_TEXT_H
