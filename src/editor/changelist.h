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

#ifndef EDITOR_CHANGELIST_H
#define EDITOR_CHANGELIST_H

#include <string>
#include <stack>
#include <vector>
#include "editor/coordinates.h"
#include "editor/update.h"

namespace Editor {
class Document;
class ChangeList {
public:
	// Forget about all of the changes.
	void clear();
	// Record a change that has been made, retaining removed line endings.
	void erase(const Range &loc, std::string text, std::vector<std::string> endings);
	void insert(const Range &loc);
	// Roll back the last transaction, or re-apply the most recently undone one.
	// Return value is the new cursor position.
	location_t undo(Document &doc, Update &update);
	location_t redo(Document &doc, Update &update);
	// End a typing group without discarding redo. Only an edit starts a branch.
	void commit();
	// Group all edits within a command into one transaction. May be nested.
	void begin();
	void end(bool finish = true);
	bool can_undo() const { return !_done.empty(); }
	bool can_redo() const { return !_undone.empty(); }
private:
	struct change_t {
		bool erased = false;
		Range loc;
		std::string text;
		std::vector<std::string> endings;
	};
	struct transaction_t {
		std::vector<change_t> changes;
	};
	void record(change_t change);
	bool can_join(const change_t &change) const;
	location_t replay(Document &doc, Update &update,
		const transaction_t &source, transaction_t &inverse);
	std::stack<transaction_t> _done;
	std::stack<transaction_t> _undone;
	// Replayed edits go here, never into the ordinary history stacks.
	transaction_t *_inverse = nullptr;
	unsigned _depth = 0;
	bool _committed = true;
};
} // namespace Editor

#endif // EDITOR_CHANGELIST_H
