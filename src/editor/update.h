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

#ifndef EDITOR_UPDATE_H
#define EDITOR_UPDATE_H

#include "coordinates.h"

namespace Editor {
class Update
{
public:
	void reset();
	void at(location_t loc);
	void at(line_t line);
	void range(const Range &range);
	void forward(location_t loc);
	void all();
	bool has_dirty() const { return _dirty; }
	bool is_dirty(line_t index) const;
private:
	bool _dirty = true;
	line_t _start = 0;
	line_t _end = 0;
};
} // namespace Editor

#endif // EDITOR_UPDATE_H
