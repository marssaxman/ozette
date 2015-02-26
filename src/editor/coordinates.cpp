//
// ozette
// Copyright (C) 2014-2015 Mars J. Saxman
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

#include "coordinates.h"

std::string Editor::location_t::to_string() const
{
	return std::to_string(line) + ":" + std::to_string(offset);
}

Editor::Range::Range(const location_t &a, const location_t &b)
{
	_begin = (a < b) ? a : b;
	_end = (a > b) ? a : b;
}

std::string Editor::Range::to_string() const
{
	return _begin.to_string() + "-" + _end.to_string();
}

bool Editor::Range::empty() const
{
	return _begin == _end;
}

bool Editor::Range::multiline() const
{
	return _begin.line != _end.line;
}

void Editor::Range::reset(const location_t &loc)
{
	_begin = loc;
	_end = loc;
}

void Editor::Range::reset(const location_t &a, const location_t &b)
{
	_begin = (a < b) ? a : b;
	_end = (a > b) ? a : b;
}

void Editor::Range::extend(const location_t &loc)
{
	_begin = (_begin < loc)? _begin: loc;
	_end = (_end > loc)? _end: loc;
}

void Editor::Range::extend(const Range &loc)
{
	if (empty()) {
		*this = loc;
	} else {
		extend(loc.begin());
		extend(loc.end());
	}
}

