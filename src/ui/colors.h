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

#ifndef UI_COLORS_H
#define UI_COLORS_H

#include <string>

namespace UI {
namespace Colors {

void init();

// colors for UI elements
int content(bool active);
int chrome(bool active);
int dialog(bool active);
int result(bool active);

// colors for syntax elements within an editor
int keyword();
int string();
int literal();
int comment();
int symbol();
int error();

} // namespace Colors
} // namespace UI

#endif //UI_COLORS_H
