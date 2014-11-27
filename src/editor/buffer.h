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

#ifndef EDITOR_BUFFER_H
#define EDITOR_BUFFER_H

#include <vector>
#include <ostream>
#include <string>
#include <memory>
#include "line.h"

namespace Editor {
class Buffer
{
public:
	void clear();
	bool empty() const;
	size_t line_count() const;
	const Line& get(size_t i) const;
	void update(size_t i, std::string text);
	void insert(size_t i, std::string text);
	void append(std::string text);
	void erase(size_t begin, size_t end);
	friend std::ostream& operator<< (std::ostream &out, const Buffer &buf);
	void commit(location_t cursor);
	static location_t undo(std::unique_ptr<Buffer> &&buf, location_t cursor);
	bool can_undo() const { return _previous.get() != nullptr; }
	static location_t redo(std::unique_ptr<Buffer> &&buf);
	bool can_redo() const { return _next.get() != nullptr; }
private:
	std::unique_ptr<Buffer> _previous;
	std::unique_ptr<Buffer> _next;
	std::vector<Line*> _lines;
	std::vector<std::unique_ptr<Line>> _storage;
	location_t _cursor;
};
} // namespace Editor

#endif //EDITOR_BUFFER_H
