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

#ifndef SEARCH_RESULT_H
#define SEARCH_RESULT_H

#include <string>

namespace Search {
struct Match {
	std::string path;
	std::string text;
	size_t index = 0;
};

class Parser {
public:
	bool read(char ch, Match &match);
	void finish();
	bool failed() const { return _failed; }
private:
	std::string _fields[3];
	unsigned _field = 0;
	bool _failed = false;
};
} // namespace Search

#endif // SEARCH_RESULT_H
