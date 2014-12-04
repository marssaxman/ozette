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

#include "changelist.h"
#include "document.h"
#include <assert.h>

void Editor::ChangeList::clear()
{
	while (!_done.empty()) _done.pop();
	while (!_undone.empty()) _undone.pop();
}

void Editor::ChangeList::erase(const Range &loc, std::string text)
{
	assert(!loc.empty());
	change_t temp;
	temp.erase = true;
	temp.eraseloc = loc;
	temp.erasetext = text;
	_done.push(temp);
}

void Editor::ChangeList::insert(const Range &loc)
{
	assert(!loc.empty());
	change_t temp;
	temp.insert = true;
	temp.insertloc = loc;
	_done.push(temp);
}

void Editor::ChangeList::split(location_t loc)
{
	change_t temp;
	temp.split = true;
	temp.splitloc = loc;
	_done.push(temp);
}

Editor::Range Editor::ChangeList::undo(Document &doc, Update &update)
{
	if (_done.empty()) return Range();
	// Remove the last change from the done list, then reverse its effect.
	change_t temp = _done.top();
	_done.pop();
	Range out = temp.rollback(doc);
	// Reversing the effect of the last change is a new change, which will go
	// onto the undo stack. That's great but this is really the inverse of a
	// change, so we will pop it off the "done" stack and put it onto the
	// "undone" stack, so that the next undo will apply to the previous "done"
	// action. We can undo the undo by invoking "redo". This is how we get to
	// have multilevel undo.
	_undone.push(_done.top());
	_done.pop();
	update.range(out);
	return out;
}

Editor::Range Editor::ChangeList::redo(Document &doc, Update &update)
{
	if (_undone.empty()) return Range();
	// Remove the most recent change from the undone list, then reverse its
	// effect. This will re-implement whatever the original change was, which
	// will push a new action onto the _done list, effectively transferring the
	// change from the "undone" list to the "done" list.
	change_t temp = _undone.top();
	_undone.pop();
	Range out = temp.rollback(doc);
	update.range(out);
	return out;
}

Editor::Range Editor::ChangeList::change_t::rollback(Document &doc)
{
	Range out;
	if (split) {
		// We inserted a linebreak at the splitloc. Delete it.
		Range span(splitloc, doc.next(splitloc));
		doc.erase(span);
		out.extend(span);
	}
	if (insert) {
		// We inserted some text, which now occupies the range specified by
		// insertloc. Delete that text.
		doc.erase(insertloc);
		out.extend(insertloc);
	}
	if (erase) {
		// We erased some text, which was at the range specified by eraseloc,
		// and which we have saved as erasetext. Re-insert it at the beginning
		// of the eraseloc.
		doc.insert(eraseloc.begin(), erasetext);
		out.extend(eraseloc);
	}
	return out;
}
