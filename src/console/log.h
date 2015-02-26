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

#ifndef CONSOLE_LOG_H
#define CONSOLE_LOG_H

#include <string>
#include <vector>

namespace Console {
class Log
{
public:
	Log(std::string command, unsigned width):
		_command(command), _width(width), _lines(1) {}
	void layout(unsigned width);
	bool read(int fd);
	bool empty() const { return _lines.empty(); }
	size_t size() const { return _lines.size(); }
	const std::string &operator[](size_t index) const { return _lines[index]; }
	const std::string &command() const { return _command; }
private:
	void read_one(char ch);
	std::string _command;
	unsigned _width;
	std::string _raw;
	std::vector<std::string> _lines;
};
} // namespace Console

#endif //CONSOLE_LOG_H
