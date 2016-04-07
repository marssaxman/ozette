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

#include "editor/update.h"
#include <climits>

void Editor::Update::reset() {
	_dirty = false;
	_start = 0;
	_end = 0;
}

void Editor::Update::at(location_t loc) {
	at(loc.line);
}

void Editor::Update::at(line_t index) {
	_start = _dirty? std::min(_start, index): index;
	_end = _dirty? std::max(_end, index): index;
	_dirty = true;
}

void Editor::Update::range(const Range &range) {
	line_t a = range.begin().line;
	line_t b = range.end().line;
	line_t from = std::min(a, b);
	line_t to = std::max(a, b);
	_start = _dirty? std::min(_start, from): from;
	_end = _dirty? std::max(_end, to): to;
	_dirty = true;
}

void Editor::Update::forward(location_t loc) {
	_start = _dirty? std::min(_start, loc.line): loc.line;
	_end = SIZE_MAX;
	_dirty = true;
}

void Editor::Update::all() {
	_dirty = true;
	_start = 0;
	_end = SIZE_MAX;
}

bool Editor::Update::is_dirty(line_t index) const {
	return _dirty && (index >= _start) && (index <= _end);
}
