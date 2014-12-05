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
	while(!_undone.empty()) _undone.pop();
	assert(!loc.empty());
	change_t temp;
	temp.erase = true;
	temp.eraseloc = loc;
	temp.erasetext = text;
	_done.push(temp);
}

void Editor::ChangeList::insert(const Range &loc)
{
	while(!_undone.empty()) _undone.pop();
	assert(!loc.empty());
	change_t temp;
	temp.insert = true;
	temp.insertloc = loc;
	_done.push(temp);
}

void Editor::ChangeList::split(location_t loc)
{
	while(!_undone.empty()) _undone.pop();
	change_t temp;
	temp.split = true;
	temp.splitloc = loc;
	_done.push(temp);
}

Editor::location_t Editor::ChangeList::undo(Document &doc, Update &update)
{
	if (_done.empty()) return location_t();
	std::stack<change_t> undone = std::move(_undone);
	// Remove the last change from the done list, then reverse its effect.
	change_t temp = _done.top();
	_done.pop();
	location_t out = temp.rollback(doc, update);
	// Reversing the effect of the last change is a new change, which will go
	// onto the undo stack. That's great but this is really the inverse of a
	// change, so we will pop it off the "done" stack and put it onto the
	// "undone" stack, so that the next undo will apply to the previous "done"
	// action. We can undo the undo by invoking "redo". This is how we get to
	// have multilevel undo.
	_undone = std::move(undone);
	_undone.push(_done.top());
	_done.pop();
	return out;
}

Editor::location_t Editor::ChangeList::redo(Document &doc, Update &update)
{
	if (_undone.empty()) return location_t();
	// Remove the most recent change from the undone list, then reverse its
	// effect. This will re-implement whatever the original change was, which
	// will push a new action onto the _done list, effectively transferring the
	// change from the "undone" list to the "done" list.
	change_t temp = _undone.top();
	_undone.pop();
	std::stack<change_t> undone = std::move(_undone);
	location_t out = temp.rollback(doc, update);
	_undone = std::move(undone);
	return out;
}

void Editor::ChangeList::commit()
{
	while (!_undone.empty()) _undone.pop();
	if (_done.empty()) return;
	_done.top().committed = true;
}

Editor::location_t Editor::ChangeList::change_t::rollback(Document &doc, Update &update)
{
	location_t out;
	if (split) {
		// We inserted a linebreak at the splitloc. Delete it.
		Range span(splitloc, doc.next(splitloc));
		doc.erase(span);
		update.range(span);
		out = splitloc;
	}
	if (insert) {
		// We inserted some text, which now occupies the range specified by
		// insertloc. Delete that text.
		doc.erase(insertloc);
		update.range(insertloc);
		out = insertloc.begin();
	}
	if (erase) {
		// We erased some text, which was at the range specified by eraseloc,
		// and which we have saved as erasetext. Re-insert it at the beginning
		// of the eraseloc.
		doc.insert(eraseloc.begin(), erasetext);
		update.range(eraseloc);
		out = eraseloc.end();
	}
	return out;
}
