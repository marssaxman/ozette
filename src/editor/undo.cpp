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

#include "undo.h"
#include "document.h"

void Editor::Undo::clear()
{
	_removeloc = Range();
	_removetext.clear();
	_insertloc = Range();
	_previous.reset(nullptr);
	_next.reset(nullptr);
}

void Editor::Undo::erase(const Range &loc, std::string text)
{
	return;
	// If we have not inserted any text, and this erase follows the previous
	// erase, we will coalesce them into a single erase operation.
	if (_insertloc.empty() && loc.begin() == _removeloc.end()) {
		_removeloc.extend(_removeloc.begin(), loc.end());
		_removetext = _removetext + text;
		return;
	}
	// If we have not inserted any text, and this erase precedes the previous
	// erase, we will coalesce them.
	if (_insertloc.empty() && loc.end() == _removeloc.begin()) {
		_removeloc.extend(loc.begin(), _removeloc.end());
		_removetext = text + _removetext;
		return;
	}
	// If we have inserted some text or there was an existing erase which is
	// not adjacent to the new one, we must push the current state and make
	// this a new action of its own.
	if (!_insertloc.empty() || !_removeloc.empty()) {
		push();
	}
	_removeloc = loc;
	_removetext = text;
}

void Editor::Undo::insert(Range loc)
{
	// If there is no inserted text, this will become the insert. If this
	// insert directly follows the previous insert, coalesce them. Otherwise,
	// push the previous edit and begin anew.
	if (_insertloc.end() == loc.begin()) {
		loc.extend(_insertloc.begin(), loc.end());
	} else if (!_insertloc.empty()) {
		//push();
		clear();
	}
	_insertloc = loc;
}

void Editor::Undo::split(location_t loc)
{
	
}

Editor::Range Editor::Undo::undo(Document &doc)
{
	Range out;
	if (!_insertloc.empty()) {
		out.reset(doc.erase(_insertloc));
	}
/*	if (!_removetext.empty()) {
		doc.insert(_removeloc.begin(), _removetext);
		if (out.empty()) {
			out = _removeloc;
		} else {
		}
	}
*/	return out;
}

Editor::Range Editor::Undo::redo(Document &doc)
{
	Range out;
	return out;
}

void Editor::Undo::push()
{
	std::unique_ptr<Undo> temp(new Undo);
	temp->_removeloc = _removeloc;
	temp->_removetext = _removetext;
	temp->_insertloc = _insertloc;
	temp->_next = std::move(_next);
	clear();
	_next = std::move(temp);
}

