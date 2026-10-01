// ozette
// Copyright (C) 2014-2018 Mars J. Saxman
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

#ifndef SEARCH_SEARCH_H
#define SEARCH_SEARCH_H

#include "ui/view.h"
#include "ui/shell.h"
#include "dialog/form.h"
#include "process/subproc.h"
#include "search/engine.h"
#include "search/result.h"
#include <set>

namespace Search {
class View : public UI::View {
public:
	static void show(UI::Shell &shell);
	static void exec(spec job, UI::Shell &shell);
	virtual void activate(UI::Frame &ctx) override;
	virtual void deactivate(UI::Frame &ctx) override;
	virtual bool process(UI::Frame &ctx, int ch) override;
	virtual bool poll(UI::Frame &ctx) override;
	virtual void set_help(UI::HelpBar::Panel &panel) override;
	virtual Priority priority() const override { return Priority::Secondary; }
protected:
	View();
	~View();
	static View *_instance;
	UI::Window *_window = nullptr;
	virtual void paint_into(WINDOW *view, State state) override;
private:
	void add_match(const Match &match);
	void read_error(char ch);
	void add_error(std::string text);
	void exec(spec job, UI::Frame &ctx);
	void ctl_kill(UI::Frame &ctx);
	void search(UI::Frame &ctx);
	void key_return(UI::Frame &ctx);
	void key_up(UI::Frame &ctx);
	void key_down(UI::Frame &ctx);
	void key_page_up(UI::Frame &ctx);
	void key_page_down(UI::Frame &ctx);
	void set_title(UI::Frame &ctx);
	unsigned maxscroll() const;
	// parsed search result data
	struct line {
		std::string text;
		std::string path;
		size_t index;
	};
	spec _job;
	std::vector<line> _lines;
	size_t _match_lines = 0;
	std::set<std::string> _match_files;
	// connection to grep
	std::unique_ptr<Process::Subproc> _proc;
	Parser _parser;
	std::string _errorbuf;
	std::string _status;
	std::string _directory;
	std::string _title;
	unsigned _scrollpos = 0;
	size_t _selection = 0;
	int _height = 0;
	int _width = 0;
};
} // namespace Search

#endif // SEARCH_SEARCH_H

