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

#include <unistd.h>
#include <stdint.h>
#include "log.h"

bool Console::Log::read(int fd)
{
	bool got_bytes = false;
	ssize_t actual = 0;
	char buf[1024];
	while ((actual = ::read(fd, buf, 1024)) > 0) {
		got_bytes = true;
		for (ssize_t i = 0; i < actual; ++i) {
			read_one(buf[i]);
		}
	}
	return got_bytes;
}

void Console::Log::read_one(char ch)
{
	switch (ch) {
		case '\n': {
			 _lines.emplace_back(std::string());
		} break;
		case '\t': {
			do {
				_lines.back().push_back(' ');
			} while (_lines.back().size() & 3);
		} break;
		default: if (isprint(ch)) {
			_lines.back().push_back(ch);
		} break;
	}
}
