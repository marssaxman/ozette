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

#include "settings.h"

// Default settings only
Editor::Settings::Settings():
	_indent_width(4)
{
}

struct INIWrap
{
	INIReader &global;
	INIReader &local;
	std::string type;
	long GetInteger(std::string key, long value)
	{
		value = global.GetInteger("", key, value);
		value = local.GetInteger("", key, value);
		value = global.GetInteger(type, key, value);
		value = local.GetInteger(type, key, value);
		return value;
	}
};

// Load settings from config files and specialize by file type
Editor::Settings::Settings(INIReader &all, INIReader &local, std::string type):
	Settings()
{
	INIWrap wrap = {all, local, type};
	long indent_width = wrap.GetInteger("indent-width", _indent_width);
	if (indent_width < 1) indent_width = 1;
	if (indent_width > 16) indent_width = 16;
	_indent_width = static_cast<unsigned>(indent_width);
}
