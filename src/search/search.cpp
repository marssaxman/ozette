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

#include "search/search.h"
#include "search/dialog.h"
#include "app/control.h"
#include "app/path.h"
#include <assert.h>
#include <algorithm>
#include <unistd.h>

Search::View *Search::View::_instance;


void Search::View::exec(spec job, UI::Shell &shell)
{
	if (_instance) {
		shell.make_active(_instance->_window);
	} else {
		_instance = new Search::View;
		std::unique_ptr<UI::View> view(_instance);
		_instance->_window = shell.open_window(std::move(view));
	}
	_instance->exec(job, *_instance->_window);
}

void Search::View::activate(UI::Frame &ctx)
{
	set_title(ctx);
}

void Search::View::deactivate(UI::Frame &ctx)
{
}

bool Search::View::process(UI::Frame &ctx, int ch)
{
	switch (ch) {
		case Control::Close: return false;
		case Control::Kill: ctl_kill(ctx); break;
		case Control::Find: ctl_find(ctx); break;
		case Control::Return: key_return(ctx); break;
		case KEY_UP: key_up(ctx); break;
		case KEY_DOWN: key_down(ctx); break;
		case KEY_PPAGE: key_page_up(ctx); break;
		case KEY_NPAGE: key_page_down(ctx); break;
	}
	set_title(ctx);
	return true;
}

bool Search::View::poll(UI::Frame &ctx)
{
	// We only need to poll if we have an active subprocess.
	if (!_proc.get()) return true;
	bool follow_edge = _scrollpos == maxscroll();
	bool got_bytes = false;
	ssize_t actual = 0;
	char buf[1024];
	// read data from the input in chunks no larger than 1K until there is no
	// more input to read
	while ((actual = ::read(_proc->out_fd(), buf, 1024)) > 0) {
		got_bytes = true;
		for (ssize_t i = 0; i < actual; ++i) {
			read_one(buf[i]);
		}
	}
	bool dirty = got_bytes;
	if (follow_edge && _scrollpos != maxscroll()) {
		_scrollpos = maxscroll();
		dirty = true;
	}
	if (!_proc->poll()) {
		_proc.reset(nullptr);
		dirty = true;
	}
	if (dirty) {
		ctx.repaint();
	}
	set_title(ctx);
	return true;
}

void Search::View::set_help(UI::HelpBar::Panel &panel)
{
	if (_proc.get()) {
		panel.label[0][0] = {"^K", "Kill"};
	} else {
		panel.label[0][5] = {"^F", "Find"};
	}
	panel.label[1][0] = {"^W", "Close"};
	panel.label[1][5] = {"^?", "Help"};
}

Search::View::View()
{
	assert(_instance == nullptr);
}

Search::View::~View()
{
	_instance = nullptr;
}

void Search::View::paint_into(WINDOW *view, State state)
{
	wmove(view, 0, 0);
	getmaxyx(view, _height, _width);

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
		if (i > 0 && i <= _lines.size()) {
			waddnstr(view, _lines[i-1].text.c_str(), _width);
		}
		wclrtoeol(view);
		if (state == State::Focused && i == 1+_selection && !_lines.empty()) {
			mvwchgat(view, row, 0, _width, A_REVERSE, 0, NULL);
		}
	}
}

void Search::View::read_one(char ch)
{
	if ('\n' == ch) {
		// we have just finished processing a line; we should have
		// between zero and three chunks in our line buffer, which
		// represent the file path, line number, and match text of
		// each match returned by grep.
		_match_lines++;
		_linebuf.resize(3);
		std::string file = _linebuf[0];
		// If this is a new file, add a new match group.
		if (_lines.empty() || _lines.back().path != file) {
			line temp = {file + ":", file, 0};
			_lines.push_back(temp);
			_match_files++;
		}
		std::string linenumber = _linebuf[1] + ":";
		std::string indent;
		if (linenumber.size() < 8) {
			indent.resize(8 - linenumber.size(), ' ');
		}
		// Line numbers are printed one-based for human consumption, but the
		// actual line numbers are of course zero-based. Since we are parsing
		// the result of some text printed by a tool intended for human use, we
		// must subtract one to get real line numbers.
		long index = std::stol(_linebuf[1]) - 1;
		if (index < 0) index = 0;
		line temp = {indent + linenumber + _linebuf[2], file, (size_t)index};
		_lines.push_back(temp);
		_linebuf.clear();
	} else if (':' == ch && _linebuf.size() < 3) {
		// as long as there are fewer than three chunks in the linebuf, add a
		// new one which will accumulate further characters on this line; we
		// divide the lines into three fields separated by colons, but the last
		// field is the match text and may include colons as part of its value.
		_linebuf.push_back(std::string());
	} else if (isprint(ch)) {
		// append the char to the last chunk in the linebuf
		if (_linebuf.empty()) {
			_linebuf.push_back(std::string());
		}
		_linebuf.back().push_back(ch);
	}
}

void Search::View::exec(spec job, UI::Frame &ctx)
{
	_job = job;
	_match_files = 0;
	_match_lines = 0;
	_selection = 0;
	_scrollpos = 0;
	_lines.clear();
	_linebuf.clear();
	std::string targetdir = job.haystack;
	if (targetdir.empty()) targetdir = ".";
	std::string findarg = " -type f";
	if (!job.filter.empty() && job.filter != "*") {
		findarg += " -name " + job.filter;
	}
	std::string find = "find " + targetdir + findarg + " -print0";
	std::string grep = "grep -H -n -I \"" + job.needle + "\"";
	std::string command = find + " | xargs -0 " + grep;
	const char *argv[1 + 2 + 1] = {"sh", "-c", command.c_str(), nullptr};
	_title = "find " + job.needle;
	if (!job.haystack.empty()) {
		_title += " in " + Path::display(job.haystack) + "/";
	}
	_proc.reset(new Console::Subproc(argv[0], argv));
	ctx.repaint();
	set_title(ctx);
}

void Search::View::ctl_kill(UI::Frame &ctx)
{
	if (_proc.get()) {
		_proc.reset(nullptr);
		ctx.repaint();
	}
}

void Search::View::ctl_find(UI::Frame &ctx)
{
	Search::Dialog::show(ctx, _job);
}

void Search::View::key_return(UI::Frame &ctx)
{
	if (_selection >= _lines.size()) return;
	auto &line = _lines[_selection];
	ctx.app().find_in_file(line.path, line.index);
}

void Search::View::key_down(UI::Frame &ctx)
{
	if (_selection < _lines.size()) {
		_selection++;
		ctx.repaint();
	}
}

void Search::View::key_up(UI::Frame &ctx)
{
	if (_selection > 0) {
		_selection--;
		ctx.repaint();
	}
}

void Search::View::key_page_down(UI::Frame &ctx)
{
	_selection = std::min(_scrollpos + (size_t)_height, _lines.size()-1);
	ctx.repaint();
}

void Search::View::key_page_up(UI::Frame &ctx)
{
	// Move to last line of previous page.
	_selection = _scrollpos > 0? _scrollpos-1: 0;
	ctx.repaint();
}

void Search::View::set_title(UI::Frame &ctx)
{
	ctx.set_title(_title);
	std::string status;
	if (_proc.get()) {
		status = "running";
	} else {
		status = std::to_string(_match_lines) + " matches in ";
		status += std::to_string(_match_files) + " files";
	}
	ctx.set_status(status);
}

unsigned Search::View::maxscroll() const
{
	// we'll show an extra blank line at the top and the bottom in order to
	// help the user see when they are at the end of the log
	int displines = (int)_lines.size() + 2;
	return (displines > _height)? (displines - _height): 0;
}

