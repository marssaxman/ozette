// ozette
// Copyright (C) 2014-2016 Mars J. Saxman
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

#include "search/search.h"
#include "search/dialog.h"
#include "app/control.h"
#include "app/path.h"
#include <assert.h>
#include <algorithm>
#include "ui/text.h"

Search::View *Search::View::_instance;

void Search::View::show(UI::Shell &shell) {
	if (_instance) {
		shell.make_active(_instance->_window);
	} else {
		_instance = new Search::View;
		std::unique_ptr<UI::View> view(_instance);
		_instance->_window = shell.open_window(std::move(view));
	}
}

void Search::View::exec(spec job, UI::Shell &shell) {
	show(shell);
	assert(_instance);
	_instance->exec(job, *_instance->_window);
}

void Search::View::activate(UI::Frame &ctx) {
	set_title(ctx);
}

void Search::View::deactivate(UI::Frame &ctx) {
}

bool Search::View::process(UI::Frame &ctx, int ch) {
	switch (ch) {
		case Control::Close: return false;
		case Control::Kill: ctl_kill(ctx); break;
		case KEY_F(4): search(ctx); break;
		case Control::Return: key_return(ctx); break;
		case KEY_UP: key_up(ctx); break;
		case KEY_DOWN: key_down(ctx); break;
		case KEY_PPAGE: key_page_up(ctx); break;
		case KEY_NPAGE: key_page_down(ctx); break;
	}
	set_title(ctx);
	return true;
}

bool Search::View::poll(UI::Frame &ctx) {
	if (!_proc) return true;
	bool follow_edge = _scrollpos == maxscroll();
	std::string output, errors;
	bool dirty = _proc->read_out(output);
	dirty |= _proc->read_err(errors);
	Match match;
	bool retry = false;
	for (auto ch: output) {
		if (_parser.read(ch, match)) add_match(match);
	}
	for (auto ch: errors) read_error(ch);
	if (!_proc->poll()) {
		_parser.finish();
		if (!_errorbuf.empty()) {
			add_error(_errorbuf);
			_errorbuf.clear();
		}
		auto result = _proc->result();
		if (result.error || result.signal || result.cancelled) {
			_status = result.message();
			add_error(_status);
		} else if (_parser.failed()) {
			_status = "search failed: " + result.message();
			add_error("invalid search result");
		} else if (result.exit_code > 1 || result.exit_code < 0) {
			_status = "search failed: " + result.message();
			add_error(_status);
		} else if (result.exit_code == 1) {
			_status = "no matches (exit 1)";
			retry = !_match_lines;
		} else {
			_status = std::to_string(_match_lines) + " matches in ";
			_status += std::to_string(_match_files.size()) + " files (exit 0)";
		}
		_proc.reset();
		dirty = true;
	}
	if (follow_edge && _scrollpos != maxscroll()) {
		_scrollpos = maxscroll();
		dirty = true;
	}
	if (dirty) ctx.repaint();
	set_title(ctx);
	if (retry) ctx.app().begin_search();
	return true;
}

void Search::View::set_help(UI::HelpBar::Panel &panel) {
	if (_proc.get()) {
		panel.kill();
	} else {
		panel.search();
	}
}

Search::View::View() {
	assert(_instance == nullptr);
}

Search::View::~View() {
	_instance = nullptr;
}

void Search::View::paint_into(WINDOW *view, State state) {
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
			Text::LineLayout layout(_lines[i-1].text, 4);
			UI::paint_text(view, row, 0, _width, layout, 0, getattrs(view));
		} else {
			wclrtoeol(view);
		}
		if (state == State::Focused && i == 1+_selection && !_lines.empty()) {
			mvwchgat(view, row, 0, _width, A_REVERSE, 0, NULL);
		}
	}
}

void Search::View::add_match(const Match &match) {
	std::string path = match.path[0] == '/'? match.path: _directory + "/" + match.path;
	if (_lines.empty() || _lines.back().path != path) {
		line header = {match.path + ":", path, 0};
		_lines.push_back(header);
		_match_files.insert(path);
	}
	std::string number = std::to_string(match.index + 1) + ":";
	std::string indent;
	if (number.size() < 8) indent.resize(8 - number.size(), ' ');
	line result = {indent + number + match.text, path, match.index};
	_lines.push_back(result);
	++_match_lines;
}

void Search::View::add_error(std::string text) {
	line error = {text, "", 0};
	_lines.push_back(error);
}

void Search::View::read_error(char ch) {
	if (ch == '\n') {
		add_error(_errorbuf);
		_errorbuf.clear();
	} else {
		_errorbuf.push_back(ch);
	}
}

void Search::View::exec(spec job, UI::Frame &ctx) {
	_job = job;
	_match_files.clear();
	_match_lines = 0;
	_selection = 0;
	_scrollpos = 0;
	_lines.clear();
	_parser = Parser();
	_errorbuf.clear();
	_status.clear();
	_directory = Path::current_dir();
	std::string filter = job.filter.empty()? "*": job.filter;
	std::string targetfile = "--include=" + filter;
	std::string targetdir = job.haystack;
	if (targetdir.empty()) targetdir = ".";
	if (targetdir[0] == '~') targetdir = Path::absolute(targetdir);
	if (targetdir[0] == '-') targetdir = "./" + targetdir;
	const char *argv[] = {
		"grep",
		"-rnHI",
		"--null",
		targetfile.c_str(),
		"-e",
		job.needle.c_str(),
		"--",
		targetdir.c_str(),
		nullptr
	};
	_title = "find " + job.needle;
	_title += " in " + filter;
	if (!job.haystack.empty()) {
		_title += " under " + Path::display(job.haystack) + "/";
	}
	_proc.reset();
	_proc.reset(new Process::Subproc(argv[0], argv));
	ctx.repaint();
	set_title(ctx);
}

void Search::View::ctl_kill(UI::Frame &ctx) {
	if (_proc.get()) {
		_proc->cancel();
		ctx.repaint();
	}
}

void Search::View::search(UI::Frame &ctx) {
	Search::Dialog::show(ctx, _job);
}

void Search::View::key_return(UI::Frame &ctx) {
	if (_selection >= _lines.size()) return;
	auto &line = _lines[_selection];
	if (!line.path.empty()) ctx.app().find_in_file(line.path, line.index);
}

void Search::View::key_down(UI::Frame &ctx) {
	if (_selection + 1 < _lines.size()) {
		_selection++;
		ctx.repaint();
	}
}

void Search::View::key_up(UI::Frame &ctx) {
	if (_selection > 0) {
		_selection--;
		ctx.repaint();
	}
}

void Search::View::key_page_down(UI::Frame &ctx) {
	if (_lines.empty()) return;
	_selection = std::min(_scrollpos + (size_t)_height, _lines.size()-1);
	ctx.repaint();
}

void Search::View::key_page_up(UI::Frame &ctx) {
	// Move to last line of previous page.
	_selection = _scrollpos > 0? _scrollpos-1: 0;
	ctx.repaint();
}

void Search::View::set_title(UI::Frame &ctx) {
	ctx.set_title(_title);
	ctx.set_status(_proc? (_proc->result().cancelled? "cancelling": "running"): _status);
}

unsigned Search::View::maxscroll() const {
	// we'll show an extra blank line at the top and the bottom in order to
	// help the user see when they are at the end of the log
	int displines = (int)_lines.size() + 2;
	return (displines > _height)? (displines - _height): 0;
}
