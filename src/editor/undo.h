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

#ifndef EDITOR_UNDO_H
#define EDITOR_UNDO_H

#include <string>
#include <memory>
#include "coordinates.h"

namespace Editor {
class Document;
class Undo
{
public:
	Undo() {}
	Undo(const Undo &other);
	void clear();
	void erase(const Range &loc, std::string text);
	void insert(Range text);
	void split(location_t loc);
	Range undo(Document &doc);
	Range redo(Document &doc);
private:
	void push();
	bool empty() const;
	Range _removeloc;
	std::string _removetext;
	Range _insertloc;
	std::unique_ptr<Undo> _previous;
	std::unique_ptr<Undo> _next;
};
} // namespace Editor

#endif //EDITOR_UNDO_H
