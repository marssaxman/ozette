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
	char indent_style() const { return _indent_style; }
	unsigned indent_size() const { return _indent_size; }
private:
	enum { TAB = '\t', SPACE = ' ' } _indent_style = TAB;
	unsigned _indent_size = 4;
	unsigned _tab_width = 0;
	enum { LF, CRLF, CR } _end_of_line = LF;
	enum { LATIN1, UTF8, UTF16BE, UTF16LE } _charset = UTF8;
	bool _trim_trailing_whitespace = true;
	bool _insert_final_newline = true;
	unsigned _max_line_length = 0;
};
} // namespace Editor

#endif //EDITOR_CONFIG_H

