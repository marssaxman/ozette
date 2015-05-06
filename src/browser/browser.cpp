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

#include "browser.h"
#include "control.h"
#include "dialog.h"
#include "picker.h"
#include "find.h"
#include <cctype>
#include <climits>
#include <assert.h>

Browser::View *Browser::View::_instance;
static std::string kExpansionStateKey = "expanded_dirs";

void Browser::View::change_directory(std::string path)
{
	if (_instance) _instance->view(path);
}

void Browser::View::open(std::string path, UI::Shell &shell)
{
	if (_instance) {
		shell.make_active(_instance->_window);
		_instance->view(path);
	} else {
		_instance = new Browser::View(path);
		std::unique_ptr<UI::View> view(_instance);
		_instance->_window = shell.open_window(std::move(view));
	}
}

Browser::View::View(std::string path):
	_tree(path)
{
	assert(_instance == nullptr);
}

void Browser::View::activate(UI::Frame &ctx)
{
	set_title(ctx);
	if (_expanded_items.empty()) {
		std::vector<std::string> paths;
		ctx.app().cache_read(kExpansionStateKey, paths);
		for (auto &path: paths) {
			_expanded_items.insert(path);
		}
		if (!paths.empty()) _rebuild_list = true;
	}
	_list.clear();
	_tree = DirTree(_tree.path());
	_rebuild_list = true;
	check_rebuild(ctx);
}

void Browser::View::deactivate(UI::Frame &ctx)
{
	std::vector<std::string> paths;
	for (auto &line: _expanded_items) {
		paths.push_back(line);
	}
	ctx.app().cache_write(kExpansionStateKey, paths);
	clear_filter(ctx);
}

void Browser::View::check_rebuild(UI::Frame &ctx)
{
	if (!_rebuild_list) return;
	build_list();
	set_title(ctx);
	_rebuild_list = false;
	ctx.repaint();
}

void Browser::View::paint_into(WINDOW *view, State state)
{
	int width;
	getmaxyx(view, _height, width);

	// adjust scrolling as necessary to keep the cursor visible
	size_t max_visible_row = _scrollpos + _height - 2;
	if (_selection < _scrollpos || _selection > max_visible_row) {
		size_t halfpage = (size_t)_height/2;
		_scrollpos = _selection - std::min(halfpage, _selection);
	}

	for (int row = 0; row < _height; ++row) {
		size_t i = row + _scrollpos;
		wmove(view, row, 0);
		// Shift by one to create a blank line at the beginning.
		if (i > 0 && i <= _list.size()) {
			whline(view, ' ', width);
			paint_row(view, row, _list[i-1], width);
		} else {
			wclrtoeol(view);
		}
		if (state == State::Focused && i == 1+_selection) {
			mvwchgat(view, row, 0, width, A_REVERSE, 0, NULL);
		}
	}
}

bool Browser::View::process(UI::Frame &ctx, int ch)
{
	switch (ch) {
		case Control::Find: ctl_find(ctx); break;
		case Control::Return: key_return(ctx); break;
		case Control::Close: return false; break;
		case Control::Escape: clear_filter(ctx); break;
		case Control::Tab: key_tab(ctx); break;
		case Control::Backspace: key_backspace(ctx); break;
		case KEY_UP: key_up(ctx); break;
		case KEY_DOWN: key_down(ctx); break;
		case KEY_PPAGE: key_page_up(ctx); break;
		case KEY_NPAGE: key_page_down(ctx); break;
		case KEY_RIGHT: key_right(ctx); break;
		case KEY_LEFT: key_left(ctx); break;
		case ' ': key_space(ctx); break;
		default: {
			if(isprint(ch)) key_char(ctx, ch);
			else clear_filter(ctx);
		} break;
	}
	return true;
}

bool Browser::View::poll(UI::Frame &ctx)
{
	check_rebuild(ctx);
	return true;
}

void Browser::View::set_help(UI::HelpBar::Panel &panel)
{
	using namespace UI::HelpBar;
	panel.label[0][0] = Label('O', true, "Open");
	panel.label[0][1] = Label('N', true, "New File");
	panel.label[0][5] = Label('F', true, "Find");
	panel.label[1][0] = Label('Q', true, "Quit");
	panel.label[1][1] = Label('E', true, "Execute");
	panel.label[1][2] = Label('D', true, "Directory");
	panel.label[1][5] = Label('?', true, "Help");
}

void Browser::View::view(std::string path)
{
	if (path == _tree.path()) return;
	_list.clear();
	_tree = DirTree(path);
	_path_filter.clear();
	_rebuild_list = true;
}

void Browser::View::paint_row(WINDOW *view, int vpos, row_t &display, int width)
{
	int rowchars = width;
	for (unsigned tab = 0; tab < display.indent; tab++) {
		waddnstr(view, "    ", rowchars);
		rowchars -= 4;
	}
	bool isdir = display.entry->is_directory();
	waddnstr(view, display.expanded? "- ": (isdir? "+ ": "  "), rowchars);
	rowchars -= 2;
	std::string name = display.entry->name();
	waddnstr(view, name.c_str(), rowchars - 1);
	rowchars -= std::min(rowchars, (int)name.size());
	waddnstr(view, isdir? "/": " ", rowchars);
	rowchars--;
	// The rest of the status info only applies to files.
	if (!display.entry->is_file()) return;

	char buf[256];
	time_t mtime = display.entry->mtime();
	struct tm mtm = *localtime(&mtime);
	time_t nowtime = time(NULL);
	struct tm nowtm = *localtime(&nowtime);
	// Pick a format string which highlights the most relevant information
	// about this file's modification date. Format strings end with an extra
	// space because it looks prettier that way.
	const char *format = "%c ";
	if (mtm.tm_year != nowtm.tm_year) {
		format = "%b %Y "; // month of year
	} else if (nowtm.tm_yday - mtm.tm_yday > 6) {
		format = "%e %b "; // day of month
	} else if (nowtm.tm_yday - mtm.tm_yday > 1) {
		format = "%a "; // day of week
	} else if (nowtm.tm_yday != mtm.tm_yday) {
		format = "yesterday ";
	} else {
		format = "%X ";	// time
	}
	size_t dchars = strftime(buf, 255, format, &mtm);
	int drawch = std::min((int)dchars, rowchars);
	mvwaddnstr(view, vpos, width-drawch, buf, drawch);
}

void Browser::View::ctl_find(UI::Frame &ctx)
{
	Find::Dialog::show(ctx);
}

void Browser::View::key_return(UI::Frame &ctx)
{
	switch (sel_entry()->type()) {
		case DirTree::Type::Directory: scope_dir(ctx); break;
		case DirTree::Type::File: edit_file(ctx); break;
		default: break;
	}
}

void Browser::View::key_up(UI::Frame &ctx)
{
	// Move to previous line in the listbox.
	if (0 == _selection) return;
	_selection--;
	ctx.repaint();
}

void Browser::View::key_down(UI::Frame &ctx)
{
	// Move to next line in the listbox.
	if (_selection + 1 == _list.size()) return;
	_selection++;
	ctx.repaint();
}

void Browser::View::key_page_up(UI::Frame &ctx)
{
	// Move to last line of previous page.
	_selection = _scrollpos > 0? _scrollpos-1: 0;
	ctx.repaint();
}

void Browser::View::key_page_down(UI::Frame &ctx)
{
	// Move to first line of next page.
	_selection = std::min(_scrollpos + _height, _list.size()-1);
	ctx.repaint();
}

void Browser::View::key_left(UI::Frame &ctx)
{
	// Move to previous match for filename filter.
	if (_path_filter.empty()) return;
	if (_selection == 0) return;
	for (size_t i = _selection; i > 0; --i) {
		if (matches_filter(i-1)) {
			_selection = i-1;
			ctx.repaint();
			return;
		}
	}
}

void Browser::View::key_right(UI::Frame &ctx)
{
	// Move to next match for filename filter.
	if (_path_filter.empty()) return;
	for (size_t i = _selection + 1; i < _list.size(); ++i) {
		if (matches_filter(i)) {
			_selection = i;
			ctx.repaint();
			return;
		}
	}
}

void Browser::View::key_space(UI::Frame &ctx)
{
	toggle(ctx);
}

void Browser::View::key_tab(UI::Frame &ctx)
{
	// If there is a name filter, collect all the names which match,
	// then extend the name filter to the common prefix of all names
	if (_path_filter.empty()) return;
	std::string prefix;
	for (size_t i = 0; i < _list.size(); ++i) {
		if (!matches_filter(i)) continue;
		std::string name = _list[i].entry->name();
		if (prefix.empty()) {
			prefix = name;
		} else while (name.substr(0, prefix.size()) != prefix) {
			prefix.pop_back();
		}
	}
	_path_filter = prefix;
	update_filter(ctx);
}

void Browser::View::key_backspace(UI::Frame &ctx)
{
	if (_path_filter.empty()) return;
	_path_filter.pop_back();
	update_filter(ctx);
}

void Browser::View::key_char(UI::Frame &ctx, char ch)
{
	_path_filter.push_back(ch);
	// Find the best match for the new filter - the name which matches the
	// filter and requires the fewest gaps to do so.
	unsigned bestlead = UINT_MAX;
	unsigned besttotal = UINT_MAX;
	for (size_t i = 0; i < _list.size(); ++i) {
		unsigned leadskip = 0;	// how many skipped leading chars?
		unsigned totalskips = 0; // how many times do we skip something?
		if (!scan_filter(i, leadskip, totalskips)) continue;
		if (leadskip > bestlead) continue;
		if (leadskip == bestlead && totalskips >= besttotal) continue;
		bestlead = leadskip;
		besttotal = totalskips;
		_selection = i;
	}
	update_filter(ctx);
}

void Browser::View::update_filter(UI::Frame &ctx)
{
	size_t lastslash = _path_filter.find_last_of('/');
	if (lastslash != std::string::npos) {
		_name_filter = _path_filter.substr(lastslash + 1);
	} else {
		_name_filter = _path_filter;
	}
	ctx.set_status(_path_filter);
	ctx.repaint();
}

void Browser::View::clear_filter(UI::Frame &ctx)
{
	if (_path_filter.empty()) return;
	_path_filter.clear();
	update_filter(ctx);
}

bool Browser::View::matches_filter(size_t index)
{
	if (index >= _list.size()) return false;
	DirTree *entry = _list[index].entry;
	std::string path = entry->path();
	size_t search = 0;
	for (char ch: _path_filter) {
		if (search == path.size()) return false;
		search = path.find_first_of(ch, search);
		if (search == std::string::npos) return false;
		++search;
	}
	return true;
}

bool Browser::View::scan_filter(
		size_t index, unsigned &leadskip, unsigned &totalskips)
{
	if (index >= _list.size()) return false;
	std::string path = _list[index].entry->path();
	size_t prev_search = 0;
	for (char ch: _path_filter) {
		if (prev_search == path.size()) return false;
		size_t next_search = path.find_first_of(ch, prev_search);
		if (next_search == std::string::npos) return false;
		if (next_search > prev_search) {
			totalskips++;
			if (prev_search == 0) {
				leadskip = next_search;
			}
		}
		prev_search = next_search + 1;
	}
	return true;
}

void Browser::View::set_title(UI::Frame &ctx)
{
	ctx.set_title(ctx.app().display_path(_tree.path()));
}

void Browser::View::build_list()
{
	_list.clear();
	insert_rows(0, 0, &_tree);
	if (_list.empty()) {
		row_t display = {0, 0, &_tree};
		_list.insert(_list.begin(), display);
	}
	_selection = std::min(_selection, _list.size());
}

void Browser::View::toggle(UI::Frame &ctx)
{
	clear_filter(ctx);
	auto &display = _list[_selection];
	auto entry = display.entry;
	if (!entry->is_directory()) return;
	if (display.expanded) {
		// collapse it
		_expanded_items.erase(display.entry->path());
		display.expanded = false;
		remove_rows(_selection + 1, display.indent + 1);
	} else {
		// expand it
		_expanded_items.insert(display.entry->path());
		display.expanded = true;
		insert_rows(_selection + 1, display.indent + 1, entry);
	}
	ctx.repaint();
}

void Browser::View::scope_dir(UI::Frame &ctx)
{
	clear_filter(ctx);
	auto &display = _list[_selection];
	auto entry = display.entry;
	if (!entry->is_directory()) return;
	if (!display.expanded) {
		_expanded_items.insert(display.entry->path());
		display.expanded = true;
		insert_rows(_selection + 1, display.indent + 1, entry);
	}
	_path_filter = ctx.app().display_path(entry->path()) + "/";
	update_filter(ctx);
	ctx.repaint();
}

void Browser::View::edit_file(UI::Frame &ctx)
{
	clear_filter(ctx);
	auto &display = _list[_selection];
	ctx.app().edit_file(display.entry->path());
}

size_t Browser::View::insert_rows(size_t index, unsigned indent, DirTree *entry)
{
	entry->scan();
	for (auto &item: entry->items()) {
		bool expand = _expanded_items.count(item.path()) > 0;
		row_t display = {indent, expand, &item};
		_list.insert(_list.begin() + index++, display);
		if (expand) {
			index = insert_rows(index, indent + 1, &item);
		}
	}
	return index;
}

void Browser::View::remove_rows(size_t index, unsigned indent)
{
	auto begin = _list.begin() + index;
	auto iter = begin;
	while (iter != _list.end() && iter->indent >= indent) {
		iter++;
	}
	_list.erase(begin, iter);
}

