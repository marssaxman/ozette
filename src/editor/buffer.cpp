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

void Editor::Buffer::clear()
{
	_lines.clear();
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
}

void Editor::Buffer::insert(size_t i, std::string text)
{
	_storage.emplace_back(new Line(text));
	_lines.emplace(_lines.begin() + i, _storage.back().get());
}

void Editor::Buffer::append(std::string text)
{
	_storage.emplace_back(new Line(text));
	_lines.emplace_back(_storage.back().get());
}

void Editor::Buffer::erase(size_t from, size_t end)
{
	auto begin = _lines.begin();
	_lines.erase(begin + from, begin + end);
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
