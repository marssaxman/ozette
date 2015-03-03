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

#include "find.h"
#include "control.h"
#include "dialog.h"
#include <assert.h>
#include <algorithm>

Find::View *Find::View::_instance;

void Find::View::exec(std::string regex, UI::Shell &shell)
{
	if (_instance) {
		shell.make_active(_instance->_window);
	} else {
		_instance = new Find::View;
		std::unique_ptr<UI::View> view(_instance);
		_instance->_window = shell.open_window(std::move(view));
	}
	_instance->exec(regex);
}

void Find::View::activate(UI::Frame &ctx)
{
	set_title(ctx);
}

void Find::View::deactivate(UI::Frame &ctx)
{
}

bool Find::View::process(UI::Frame &ctx, int ch)
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

bool Find::View::poll(UI::Frame &ctx)
{
	// We only need to poll if we have an active subprocess.
	if (!_proc.get()) return true;
	bool follow_edge = _scrollpos == maxscroll();
	bool dirty = _log->read(_proc->out_fd());
	dirty |= _log->read(_proc->err_fd());
	if (follow_edge && _scrollpos != maxscroll()) {
		_scrollpos = maxscroll();
		dirty = true;
	}
	if (dirty) {
		ctx.repaint();
	}
	if (!_proc->poll()) {
		_proc.reset(nullptr);
	}
	set_title(ctx);
	return true;
}

void Find::View::set_help(UI::HelpBar::Panel &panel)
{
	if (_proc.get()) {
		panel.label[0][0] = UI::HelpBar::Label('K', true, "Kill");
	}
	panel.label[1][0] = UI::HelpBar::Label('W', true, "Close");
	panel.label[1][5] = UI::HelpBar::Label('?', true, "Help");
}

Find::View::View()
{
	assert(_instance == nullptr);
}

Find::View::~View()
{
	_instance = nullptr;
}

void Find::View::paint_into(WINDOW *view, State state)
{
	if (!_log.get()) return;
	wmove(view, 0, 0);
	getmaxyx(view, _height, _width);
	_log->layout(_width);

	// adjust scrolling as necessary to keep the cursor visible
	size_t max_visible_row = _scrollpos + _height - 2;
	if (_selection < _scrollpos || _selection > max_visible_row) {
		size_t halfpage = (size_t)_height/2;
		_scrollpos = _selection - std::min(halfpage, _selection);
	}

	for (int row = 0; row < _height; ++row) {
		wmove(view, row, 0);
		size_t i = row + _scrollpos;
		// Sub one to create a blank leading line
		if (i > 0 && i <= _log->size()) {
			waddnstr(view, (*_log)[i-1].c_str(), _width);
		}
		wclrtoeol(view);
		if (state == State::Focused && i == 1+_selection) {
			mvwchgat(view, row, 0, _width, A_REVERSE, 0, NULL);
		}
	}
}

void Find::View::exec(std::string regex)
{
	// Set up for the old console exec code.
	std::string find = "find . -type f -print0";
	std::string grep = "grep -H -n -I \"" + regex + "\"";
	std::string command = find + " | xargs -0 " + grep;
	const char *argv[1 + 2 + 1] = {"sh", "-c", command.c_str(), nullptr};
	std::string title = "find: " + regex;
	_proc.reset(new Console::Subproc(argv[0], argv));
	_selection = 0;
	_scrollpos = 0;
	_log.reset(new Console::Log(title, _width));
}

void Find::View::ctl_kill(UI::Frame &ctx)
{
	if (_proc.get()) {
		_proc.reset(nullptr);
		ctx.repaint();
	}
}

void Find::View::key_down(UI::Frame &ctx)
{
	if (_selection < _log->size()) {
		_selection++;
		ctx.repaint();
	}
}

void Find::View::key_up(UI::Frame &ctx)
{
	if (_selection > 0) {
		_selection--;
		ctx.repaint();
	}
}

void Find::View::set_title(UI::Frame &ctx)
{
	std::string title = (_log.get())? _log->command(): "find";
	ctx.set_title(title);
	ctx.set_status(_proc.get()? "running": "");
}

unsigned Find::View::maxscroll() const
{
	// we'll show an extra blank line at the top and the bottom in order to
	// help the user see when they are at the end of the log
	int displines = (int)_log->size() + 2;
	return (displines > _height)? (displines - _height): 0;
}
