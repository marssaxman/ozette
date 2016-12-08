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

// This is an implementation of the editorconfig standard:
//     http://www.editorconfig.org/
// Any behavior divergent from that specification is unintentional.

namespace Editor {
class Config {
public:
	Config() { reset(); }
	void load(std::string path);
	// Properties which control behaviors that this editor actually implements:
	char indent_style() const { return _indent_style; }
	unsigned indent_size() const { return _indent_size; }
	// Other properties are supported, as per the standard, but have no effect.
private:
	void reset();
	void apply(std::string key, std::string val);
	enum { TAB = '\t', SPACE = ' ' } _indent_style;
	unsigned _indent_size;
	unsigned _tab_width;
	enum { LF, CRLF, CR } _end_of_line;
	enum { LATIN1, UTF8, UTF8BOM, UTF16BE, UTF16LE } _charset;
	bool _trim_trailing_whitespace;
	bool _insert_final_newline;
	unsigned _max_line_length;
};
} // namespace Editor

#endif //EDITOR_CONFIG_H

