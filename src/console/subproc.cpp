// ozette
// Copyright (C) 2015-2016 Mars J. Saxman
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

#include "console/subproc.h"
#include <cctype>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include "console/popenRWE.h"
#include <assert.h>
#include <unistd.h>

Console::Subproc::Subproc(const char *exe, const char **argv) {
	_pid = popenRWE(_rwepipe, exe, argv);
	if (_pid > 0) {
		int ourpid = getpid();
		for (unsigned i = 0; i <= 2; ++i) {
			int fd = _rwepipe[i];
			assert(-1 != fcntl(fd, F_SETOWN, ourpid));
			int fl = fcntl(fd, F_GETFL);
			assert(fl >= 0);
			assert(-1 != fcntl(fd, F_SETFL, fl | O_ASYNC | O_NONBLOCK));
		}
	}
}

bool Console::Subproc::poll() {
	if (waitpid(_pid, nullptr, WNOHANG) == _pid) {
		close();
	}
	return _pid > 0;
}

void Console::Subproc::close() {
	if (0 >= _pid) return;
	kill(_pid, SIGTERM);
	pcloseRWE(_pid, _rwepipe);
	_pid = 0;
}

