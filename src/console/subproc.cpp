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

#include "subproc.h"
#include <cctype>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include "popenRWE.h"

static void set_nonblocking(int fd)
{
	fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
}

void Console::Subproc::open(const char *exe, const char **argv)
{
	_pid = popenRWE(_rwepipe, exe, argv);
	if (_pid > 0) {
		set_nonblocking(_rwepipe[0]);
		set_nonblocking(_rwepipe[1]);
		set_nonblocking(_rwepipe[2]);
	}
}

void Console::Subproc::poll()
{
	if (waitpid(_pid, nullptr, WNOHANG) == _pid) {
		close();
	}
}

void Console::Subproc::close()
{
	if (0 >= _pid) return;
	kill(_pid, SIGTERM);
	pcloseRWE(_pid, _rwepipe);
	_pid = 0;
}

