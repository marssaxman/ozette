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

#ifndef EDITOR_SETTINGS_H
#define EDITOR_SETTINGS_H

#include "config.h"

namespace Editor {
class Settings {
public:
	Settings() {}
	Settings(const Config &config);
	unsigned indent_width() const { return _indent_width; }
private:
	unsigned _indent_width = 4; // range 1..16
};
} // namespace Editor

#endif //EDITOR_SETTINGS_H

