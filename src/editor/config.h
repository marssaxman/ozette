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

#ifndef EDITOR_CONFIG_H
#define EDITOR_CONFIG_H

#include <string>

namespace Editor {
class Config {
public:
	Config() {}
	void load(std::string path) {} // load config settings for this file
	bool indent_with_tabs() const { return _indent_with_tabs; }
	unsigned indent_size() const { return _indent_size; }
private:
	bool _indent_with_tabs = true;
	unsigned _indent_size = 4;
};
} // namespace Editor

#endif //EDITOR_CONFIG_H

