//
// ozette
// Copyright (C) 2014-2015 Mars J. Saxman
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

#include <unistd.h>
#include <stdint.h>
#include "log.h"

void Console::Log::layout(unsigned width)
{
	if (_width == width) return;
	_width = width;
	_lines.clear();
	_lines.emplace_back(std::string());
	for (auto ch: _raw) {
		read_one(ch);
	}
}

bool Console::Log::read(int fd)
{
	bool got_bytes = false;
	ssize_t actual = 0;
	char buf[1024];
	while ((actual = ::read(fd, buf, 1024)) > 0) {
		got_bytes = true;
		_raw.append(buf, actual);
		for (ssize_t i = 0; i < actual; ++i) {
			read_one(buf[i]);
		}
	}
	return got_bytes;
}

void Console::Log::read_one(char ch)
{
	std::string &tail = _lines.back();
	switch (ch) {
		case '\n': {
			 _lines.emplace_back(std::string());
		} break;
		case '\t': {
			do {
				tail.push_back(' ');
			} while (tail.size() & 3);
		} break;
		default: if (isprint(ch)) {
			if (tail.size() >= _width) {
				_lines.emplace_back("    " + std::string(1, ch));
			} else {
				tail.push_back(ch);
			}
		} break;
	}
}
