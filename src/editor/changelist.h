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

#ifndef EDITOR_CHANGELIST_H
#define EDITOR_CHANGELIST_H

#include <string>
#include <stack>
#include "coordinates.h"
#include "update.h"

namespace Editor {
class Document;
class ChangeList
{
public:
	// Forget about all of the changes
	void clear();
	// Record a change that has been made
	void erase(const Range &loc, std::string text);
	void insert(const Range &loc);
	void split(location_t loc);
	// Roll back the last change, or re-apply the most recently undone change.
	// Return value is the new cursor position.
	location_t undo(Document &doc, Update &update);
	location_t redo(Document &doc, Update &update);
private:
	struct change_t
	{
		location_t rollback(Document &doc, Update &update);
		bool erase = false;
		Range eraseloc;
		std::string erasetext;
		bool insert = false;
		Range insertloc;
		bool split = false;
		location_t splitloc;
	};
	std::stack<change_t> _done;
	std::stack<change_t> _undone;
};
} // namespace Editor

#endif //EDITOR_CHANGELIST_H
