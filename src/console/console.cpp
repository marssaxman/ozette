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
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include "popenRWE.h"

Console::View *Console::View::_instance;

void Console::View::exec(std::string cmd, UI::Shell &shell)
{
	std::vector<std::string> args = {"-c", cmd};
	exec(cmd, "sh", args, shell);
}

void Console::View::exec(
		std::string title,
		std::string exe,
		const std::vector<std::string> &argv,
		UI::Shell &shell)
{
	if (_instance) {
		shell.make_active(_instance->_window);
	} else {
		std::unique_ptr<UI::View> view(new Console::View());
		UI::Shell::Position pos = UI::Shell::Position::Secondary;
		_instance->_window = shell.open_window(std::move(view), pos);
	}
	_instance->exec(title, exe, argv);
}

void Console::View::activate(UI::Frame &ctx)
{
	set_title(ctx);
}

void Console::View::deactivate(UI::Frame &ctx)
{
}

bool Console::View::process(UI::Frame &ctx, int ch)
{
	switch (ch) {
		case Control::Close: return false;
		case Control::Kill: ctl_kill(ctx); break;
		case KEY_UP: key_up(ctx); break;
		case KEY_DOWN: key_down(ctx); break;
	}
	set_title(ctx);
	return true;
}

bool Console::View::poll(UI::Frame &ctx)
{
	// We only need to poll if we have an active subprocess.
	if (_subpid <= 0) return true;
	bool follow_edge = _scrollpos == maxscroll();
	//int sub_stdin = _rwepipe[0];
	int sub_stdout = _rwepipe[1];
	int sub_stderr = _rwepipe[2];
	bool dirty = _log->read(sub_stdout);
	dirty |= _log->read(sub_stderr);
	if (follow_edge && _scrollpos != maxscroll()) {
		_scrollpos = maxscroll();
		dirty = true;
	}
	if (dirty) {
		ctx.repaint();
	}
	if (waitpid(_subpid, nullptr, WNOHANG) == _subpid) {
		close_subproc();
	}
	set_title(ctx);
	return true;
}

void Console::View::set_help(UI::HelpBar::Panel &panel)
{
	panel.label[0][0] = UI::HelpBar::Label('K', true, "Kill");
	panel.label[1][0] = UI::HelpBar::Label('W', true, "Close");
	panel.label[1][1] = UI::HelpBar::Label('E', true, "Execute");
	panel.label[1][5] = UI::HelpBar::Label('?', true, "Help");
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
	getmaxyx(view, _height, _width);
	_log->layout(_width);
	for (int row = 0; row < _height; ++row) {
		wmove(view, row, 0);
		size_t i = row + _scrollpos;
		// Sub one to create a blank leading line
		if (i > 0 && i <= _log->size()) {
			waddnstr(view, (*_log)[i-1].c_str(), _width);
		}
		wclrtoeol(view);
	}
}

static void set_nonblocking(int fd)
{
	fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
}

void Console::View::exec(std::string title, std::string exe, const std::vector<std::string> &args)
{
	close_subproc();
	// Convert this vector into an old-style C array of chars.
	// Allocate one extra slot at the beginning for the exe name,
	// and one extra at the end to serve as terminator.
	const char **argv = new const char*[1+args.size()+1];
	unsigned i = 0;
	argv[i++] = exe.c_str();
	for (auto &arg: args) {
		argv[i++] = arg.c_str();
	}
	argv[i] = nullptr;
	_subpid = popenRWE(_rwepipe, exe.c_str(), argv);
	delete[] argv;
	if (_subpid > 0) {
		set_nonblocking(_rwepipe[0]);
		set_nonblocking(_rwepipe[1]);
		set_nonblocking(_rwepipe[2]);
	}
	_log.reset(new Log(title, _width));
}

void Console::View::close_subproc()
{
	if (0 >= _subpid) return;
	kill(_subpid, SIGTERM);
	pcloseRWE(_subpid, _rwepipe);
	_subpid = 0;
}

void Console::View::ctl_kill(UI::Frame &ctx)
{
	if (0 == _subpid) return;
	close_subproc();
	ctx.repaint();
}

void Console::View::key_down(UI::Frame &ctx)
{
	if (_scrollpos < maxscroll()) {
		_scrollpos++;
		ctx.repaint();
	}
}

void Console::View::key_up(UI::Frame &ctx)
{
	if (_scrollpos > 0) {
		_scrollpos--;
		ctx.repaint();
	}
}

void Console::View::set_title(UI::Frame &ctx)
{
	std::string title = (_log.get())? _log->command(): "Console";
	ctx.set_title(title);
	ctx.set_status(_subpid != 0? "running": "");
}

unsigned Console::View::maxscroll() const
{
	// we'll show an extra blank line at the top and the bottom in order to
	// help the user see when they are at the end of the log
	int displines = (int)_log->size() + 2;
	return (displines > _height)? (displines - _height): 0;
}
