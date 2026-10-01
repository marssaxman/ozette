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

#include "process/subproc.h"
#include "process/popenRWE.h"
#include <cerrno>
#include <cstring>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace {
std::vector<int> abandoned;

void close_fd(int &fd) {
	if (fd >= 0) ::close(fd);
	fd = -1;
}
} // namespace

std::string Process::Result::message() const {
	if (error) return "process error: " + std::string(strerror(error));
	if (cancelled) return "cancelled";
	if (signal) return "signal " + std::to_string(signal);
	return "exit " + std::to_string(exit_code);
}

Process::Subproc::Subproc(const char *exe, const char **argv) {
	_pid = popenRWE(_rwepipe, exe, argv);
	if (_pid < 0) {
		_result.error = errno;
		_pid = 0;
	}
	close_fd(_rwepipe[0]);
}

Process::Subproc::~Subproc() {
	if (_pid > 0) {
		kill(-_pid, SIGKILL);
		abandoned.push_back(_pid);
	}
	for (auto &fd: _rwepipe) close_fd(fd);
	reap();
}

void Process::Subproc::reap() {
	for (auto it = abandoned.begin(); it != abandoned.end();) {
		int rc = waitpid(*it, nullptr, WNOHANG);
		if (rc == *it || (rc < 0 && errno == ECHILD)) {
			it = abandoned.erase(it);
		} else {
			++it;
		}
	}
}

void Process::Subproc::cancel() {
	if (_pid <= 0 || _result.cancelled) return;
	_result.cancelled = true;
	_deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(500);
	if (kill(-_pid, SIGTERM) < 0 && errno != ESRCH) _result.error = errno;
}

bool Process::Subproc::poll() {
	if (_pid > 0) {
		if (_result.cancelled && !_killed && std::chrono::steady_clock::now() >= _deadline) {
			if (kill(-_pid, SIGKILL) < 0 && errno != ESRCH) _result.error = errno;
			_killed = true;
		}
		int status = 0;
		int rc = waitpid(_pid, &status, WNOHANG);
		if (rc == _pid || (rc < 0 && errno != EINTR)) {
			if (rc < 0) {
				_result.error = errno;
			} else if (WIFEXITED(status)) {
				_result.exit_code = WEXITSTATUS(status);
			} else if (WIFSIGNALED(status)) {
				_result.signal = WTERMSIG(status);
			}
			if (rc == _pid) kill(-_pid, SIGKILL);
			_pid = 0;
		}
	}
	return _pid > 0 || _rwepipe[1] >= 0 || _rwepipe[2] >= 0;
}

bool Process::Subproc::read(unsigned stream, std::string &text) {
	int &fd = _rwepipe[stream];
	if (fd < 0) return false;
	size_t initial = text.size();
	char buf[4096];
	for (unsigned remaining = 16384; remaining > 0;) {
		ssize_t actual = ::read(fd, buf, sizeof(buf));
		if (actual > 0) {
			text.append(buf, actual);
			remaining -= actual;
		} else if (actual == 0) {
			close_fd(fd);
			break;
		} else if (errno != EINTR) {
			if (errno != EAGAIN && errno != EWOULDBLOCK) {
				_result.error = errno;
				close_fd(fd);
				cancel();
			}
			break;
		}
	}
	return text.size() != initial;
}
