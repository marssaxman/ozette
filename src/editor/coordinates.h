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

#ifndef EDITOR_COORDINATES_H
#define EDITOR_COORDINATES_H

#include <cstddef>
#include <string>

namespace Editor {

// Documents are a densely-packed ascending sequence of lines.
// Each line consists of a sequence of characters.
// A byte offset locates the beginning of such a character.
typedef size_t line_t;
typedef size_t offset_t;
struct location_t {
	location_t() {}
	location_t(line_t l, offset_t o): line(l), offset(o) {}
	std::string to_string() const;
	line_t line = 0;
	offset_t offset = 0;
};

// A location range identifies some sequence of characters.
class Range {
public:
	Range() {}
	Range(const location_t &a, const location_t &b);
	std::string to_string() const;
	const location_t &begin() const { return _begin; }
	const location_t &end() const { return _end; }
	bool empty() const;
	bool multiline() const;
	void reset(const location_t &loc);
	void reset(const location_t &a, const location_t &b);
	void extend(const location_t &loc);
	void extend(const Range &loc);
private:
	location_t _begin;
	location_t _end;
};

// A display space is a two-dimensional plane made up of
// equally sized cells organized in rows and columns.
typedef unsigned row_t;
typedef unsigned column_t;
struct position_t {
	row_t v;
	column_t h;
};

} // namespace Editor

inline bool operator==(
		const Editor::location_t &lhs, const Editor::location_t &rhs) {
	return lhs.line == rhs.line && lhs.offset == rhs.offset;
}

inline bool operator!=(
		const Editor::location_t& lhs, const Editor::location_t& rhs) {
	return !operator==(lhs,rhs);
}

inline bool operator< (
		const Editor::location_t& lhs, const Editor::location_t& rhs) {
	if (lhs.line < rhs.line) return true;
	if (lhs.line > rhs.line) return false;
	return lhs.offset < rhs.offset;
}

inline bool operator> (
		const Editor::location_t& lhs, const Editor::location_t& rhs) {
	return  operator< (rhs,lhs);
}

inline bool operator<=(
		const Editor::location_t& lhs, const Editor::location_t& rhs) {
	return !operator> (lhs,rhs);
}

inline bool operator>=(
		const Editor::location_t& lhs, const Editor::location_t& rhs) {
	return !operator< (lhs,rhs);
}

#endif // EDITOR_COORDINATES_H

