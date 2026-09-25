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

#include "editor/changelist.h"
#include "editor/document.h"
#include <assert.h>

void Editor::ChangeList::clear() {
	assert(!_depth && !_inverse);
	_done = {};
	_undone = {};
	_committed = true;
	_position = _saved = _next_position = 0;
}

void Editor::ChangeList::erase(const Range &loc, std::string text,
		std::vector<std::string> endings) {
	assert(!loc.empty());
	change_t change;
	change.erased = true;
	change.loc = loc;
	change.text = std::move(text);
	change.endings = std::move(endings);
	record(std::move(change));
}

void Editor::ChangeList::insert(const Range &loc) {
	assert(!loc.empty());
	change_t change;
	change.loc = loc;
	record(std::move(change));
}

void Editor::ChangeList::record(change_t change) {
	if (_inverse) {
		_inverse->changes.push_back(std::move(change));
		return;
	}
	_undone = {};
	if (_committed || (!_depth && !can_join(change))) {
		_done.push(transaction_t());
		_done.top().before = _position;
	}
	auto &changes = _done.top().changes;
	if (!changes.empty() && !change.erased && !changes.back().erased &&
			changes.back().loc.end() == change.loc.begin()) {
		// A run of typing needs only its combined range, not one record per byte.
		changes.back().loc.extend(change.loc.end());
	} else {
		changes.push_back(std::move(change));
	}
	_committed = false;
	_done.top().after = _position = ++_next_position;
}

bool Editor::ChangeList::can_join(const change_t &change) const {
	if (_done.empty()) return false;
	const auto &last = _done.top().changes.back();
	if (last.erased != change.erased) return false;
	if (!change.erased) return last.loc.end() == change.loc.begin();
	// Forward deletes reuse the same cursor; backspaces move toward the start.
	return last.loc.begin() == change.loc.begin() ||
		last.loc.begin() == change.loc.end();
}

Editor::location_t Editor::ChangeList::undo(Document &doc, Update &update) {
	assert(!_depth);
	commit();
	if (_done.empty()) return location_t();
	transaction_t inverse;
	location_t out = replay(doc, update, _done.top(), inverse);
	_done.pop();
	_undone.push(std::move(inverse));
	return out;
}

Editor::location_t Editor::ChangeList::redo(Document &doc, Update &update) {
	assert(!_depth);
	commit();
	if (_undone.empty()) return location_t();
	transaction_t inverse;
	location_t out = replay(doc, update, _undone.top(), inverse);
	_undone.pop();
	_done.push(std::move(inverse));
	return out;
}

void Editor::ChangeList::commit() {
	if (!_depth) _committed = true;
}

void Editor::ChangeList::mark_saved() {
	assert(!_depth && !_inverse);
	commit();
	_saved = _position;
}

void Editor::ChangeList::begin() {
	commit();
	++_depth;
}

void Editor::ChangeList::end(bool finish) {
	assert(_depth);
	--_depth;
	if (finish) commit();
}

Editor::location_t Editor::ChangeList::replay(Document &doc, Update &update,
		const transaction_t &source, transaction_t &inverse) {
	assert(!_inverse);
	inverse.before = source.after;
	inverse.after = source.before;
	_inverse = &inverse;
	location_t out;
	try {
		for (auto it = source.changes.rbegin(); it != source.changes.rend(); ++it) {
			if (it->erased) {
				out = doc.insert(it->loc.begin(), it->text, it->endings);
			} else {
				out = doc.erase(it->loc);
			}
			if (it->loc.multiline()) update.forward(it->loc.begin());
			else update.range(it->loc);
		}
	} catch (...) {
		_inverse = nullptr;
		throw;
	}
	_inverse = nullptr;
	_position = source.before;
	return out;
}
