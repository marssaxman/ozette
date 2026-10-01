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

#include "search/result.h"
#include <limits>

bool Search::Parser::read(char ch, Match &match) {
	if (_field == 0 && ch == '\0') {
		_field = 1;
	} else if (_field == 1 && ch == ':') {
		_field = 2;
	} else if (_field > 0 && ch == '\n') {
		size_t number = 0;
		bool valid = _field == 2 && !_fields[0].empty() && !_fields[1].empty();
		for (char digit: _fields[1]) {
			if (digit < '0' || digit > '9' ||
				number > (std::numeric_limits<size_t>::max() - (digit - '0')) / 10) {
				valid = false;
				break;
			}
			number = number * 10 + (digit - '0');
		}
		valid &= number > 0;
		if (valid) {
			match.path = std::move(_fields[0]);
			match.text = std::move(_fields[2]);
			match.index = number - 1;
		} else {
			_failed = true;
		}
		for (auto &field: _fields) field.clear();
		_field = 0;
		return valid;
	} else {
		_fields[_field].push_back(ch);
	}
	return false;
}

void Search::Parser::finish() {
	if (_field || !_fields[0].empty()) _failed = true;
}
