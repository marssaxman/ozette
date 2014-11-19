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

#include "console.h"
#include "control.h"
#include "dialog.h"
#include <cctype>
#include "popenRWE.h"

Console::View *Console::View::_instance;

void Console::View::exec(std::string cmd, UI::Shell &shell)
{
	if (_instance) {
		shell.make_active(_instance->_window);
	} else {
		std::unique_ptr<UI::View> view(new Console::View());
		_instance->_window = shell.open_window(std::move(view));
	}
	_instance->exec(cmd);
}

void Console::View::activate(UI::Frame &ctx)
{
}

void Console::View::deactivate(UI::Frame &ctx)
{
}

bool Console::View::process(UI::Frame &ctx, int ch)
{
	switch (ch) {
		case Control::Close: return false;
	}
	return true;
}

bool Console::View::poll(UI::Frame &ctx)
{
	// We only need to poll if we have an active subprocess.
	if (_subpid <= 0) return true;
	//int sub_stdin = _rwepipe[0];
	int sub_stdout = _rwepipe[1];
	//int sub_stderr = _rwepipe[2];
	bool dirty = _log->read(sub_stdout);
	if (dirty) {
		ctx.repaint();
	}
	return true;
}

Console::View::~View()
{
	_instance = nullptr;
	close_subproc();
}

void Console::View::paint_into(WINDOW *view, bool active)
{
	if (!_log.get()) return;
	wmove(view, 0, 0);
	int height, width;
	getmaxyx(view, height, width);
	for (int row = 0; row < height; ++row) {
		wmove(view, row, 0);
		size_t i = row;
		if (i < _log->size()) {
			waddnstr(view, (*_log)[i].c_str(), width);
		}
		wclrtoeol(view);
	}
}

void Console::View::exec(std::string cmd)
{
	close_subproc();
	// Parse the command string, extracting the executable path and the
	// arguments vector.
	std::vector<std::string> segs;
	size_t off = 0;
	size_t next = cmd.find_first_of(' ');
	while (next != std::string::npos) {
		segs.push_back(cmd.substr(off, next-off));
		off = next + 1;
		next = cmd.find_first_of(' ', off);
	}
	segs.push_back(cmd.substr(off));
	// Convert this vector into an old-style C array of chars.
	const char *exe = segs[0].c_str();
	const char **argv = new const char*[segs.size()+1];
	unsigned i = 0;
	for (auto &seg: segs) {
		argv[i++] = seg.c_str();
	}
	argv[i] = nullptr;
	_subpid = popenRWE(_rwepipe, exe, argv);
	delete[] argv;
	_log.reset(new Log(cmd));
}

void Console::View::close_subproc()
{
	if (0 <= _subpid) return;
	pcloseRWE(_subpid, _rwepipe);
	_subpid = 0;
}

