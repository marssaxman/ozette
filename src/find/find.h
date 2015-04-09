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

#ifndef FIND_FIND_H
#define FIND_FIND_H

#include "view.h"
#include "shell.h"
#include "subproc.h"
#include <set>

namespace Find {
class View : public UI::View
{
public:
	static void exec(std::string regex, UI::Shell &shell);
	virtual void activate(UI::Frame &ctx) override;
	virtual void deactivate(UI::Frame &ctx) override;
	virtual bool process(UI::Frame &ctx, int ch) override;
	virtual bool poll(UI::Frame &ctx) override;
	virtual void set_help(UI::HelpBar::Panel &panel) override;
	virtual Priority priority() const { return Priority::Secondary; }
protected:
	View();
	~View();
	static View *_instance;
	UI::Window *_window = nullptr;
	virtual void paint_into(WINDOW *view, State state) override;
private:
	void read_one(char ch);
	void exec(std::string regex);
	void ctl_kill(UI::Frame &ctx);
	void ctl_return(UI::Frame &ctx);
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
	std::vector<line> _lines;
	unsigned _match_lines = 0;
	unsigned _match_files = 0;
	// connection to the shell running find and grep
	std::unique_ptr<Console::Subproc> _proc;
	// linebuf is temporary storage used while reading data from _proc
	std::vector<std::string> _linebuf;
	std::string _title;
	unsigned _scrollpos = 0;
	size_t _selection = 0;
	int _height = 0;
	int _width = 0;
};
} // namespace Find

#endif // FIND_FIND_H

