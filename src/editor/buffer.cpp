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

#include "buffer.h"
#include <assert.h>

void Editor::Buffer::clear()
{
	if (_lines.empty()) return;
	_lines.clear();
	_changed = true;
}

bool Editor::Buffer::empty() const
{
	return _lines.empty();
}

size_t Editor::Buffer::line_count() const
{
	return _lines.size();
}

const Editor::Line &Editor::Buffer::get(size_t i) const
{
	return *_lines[i];
}

void Editor::Buffer::update(size_t i, std::string text)
{
	_storage.emplace_back(new Line(text));
	_lines[i] = _storage.back().get();
	_changed = true;
}

void Editor::Buffer::insert(size_t i, std::string text)
{
	_storage.emplace_back(new Line(text));
	_lines.emplace(_lines.begin() + i, _storage.back().get());
	_changed = true;
}

void Editor::Buffer::append(std::string text)
{
	_storage.emplace_back(new Line(text));
	_lines.emplace_back(_storage.back().get());
	_changed = true;
}

void Editor::Buffer::erase(size_t from, size_t end)
{
	if (from == end) return;
	auto begin = _lines.begin();
	_lines.erase(begin + from, begin + end);
	_changed = true;
}

namespace Editor {
std::ostream &operator<< (std::ostream &out, const Buffer &buffer)
{
	for (auto &line: buffer._lines) {
		out << line->text() << std::endl;
	}
	return out;
}
} // namespace Editor

void Editor::Buffer::commit(location_t cursor)
{
	if (!_changed) return;
	// An undoable action has completed.
	// Save the current state as a "previous" object, then reset the current
	// object's state.  Transfer ownership of our lines to the new object, and
	// make sure the new object has the same array of lines we do.
	_next.reset(nullptr);
	std::unique_ptr<Buffer> prevprev = std::move(_previous);
	_previous.reset(new Buffer);
	_previous->_cursor = cursor;
	_previous->_previous = std::move(prevprev);
	_previous->_storage = std::move(_storage);
	_previous->_lines = _lines;
}

Editor::location_t Editor::Buffer::undo(std::unique_ptr<Buffer> &&buf)
{
	if (!buf->_previous.get()) return buf->_cursor;
	std::unique_ptr<Buffer> temp = std::move(buf);
	buf = std::move(temp->_previous);
	buf->_next = std::move(temp);
	return buf->_cursor;
}

Editor::location_t Editor::Buffer::redo(std::unique_ptr<Buffer> &&buf)
{
	if (!buf->_next.get()) return buf->_cursor;
	std::unique_ptr<Buffer> temp = std::move(buf);
	buf = std::move(temp->_next);
	buf->_previous = std::move(temp);
	return buf->_cursor;
}
