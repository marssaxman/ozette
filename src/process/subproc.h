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

#ifndef PROCESS_SUBPROC_H
#define PROCESS_SUBPROC_H

#include <chrono>
#include <string>

namespace Process {
struct Result {
	int error = 0;
	int exit_code = -1;
	int signal = 0;
	bool cancelled = false;
	std::string message() const;
};

class Subproc {
public:
	Subproc(const char *exe, const char **argv);
	~Subproc();
	Subproc(const Subproc &) = delete;
	Subproc &operator=(const Subproc &) = delete;
	bool poll();
	void cancel();
	bool read_out(std::string &text) { return read(1, text); }
	bool read_err(std::string &text) { return read(2, text); }
	const Result &result() const { return _result; }
	static void reap();
private:
	bool read(unsigned stream, std::string &text);
	int _rwepipe[3] = {-1,-1,-1};
	int _pid = 0;
	bool _killed = false;
	std::chrono::steady_clock::time_point _deadline;
	Result _result;
};
} // namespace Process

#endif // PROCESS_SUBPROC_H
