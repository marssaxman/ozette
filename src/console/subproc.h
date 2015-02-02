//
// lindi
// Copyright (C) 2015 Mars J. Saxman
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

#ifndef CONSOLE_SUBPROC_H
#define CONSOLE_SUBPROC_H

namespace Console {
class Subproc
{
public:
	Subproc(const char *exe, const char **argv);
	~Subproc() { close(); }
	bool poll();
	// stdin/stdout/stderr from the subprocess' point of view
	int in_fd() const { return _rwepipe[0]; }
	int out_fd() const { return _rwepipe[1]; }
	int err_fd() const { return _rwepipe[2]; }
private:
	void close();
	int _rwepipe[3] = {0,0,0};
	int _pid = 0;
};
} // namespace Console

#endif // CONSOLE_SUBPROC_H

