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

#ifndef CONSOLE_CONSOLE_H
#define CONSOLE_CONSOLE_H

#include "ui/view.h"
#include "ui/shell.h"
#include "console/log.h"
#include "console/subproc.h"
#include <memory>

namespace Console {
class View : public UI::View {
public:
	static void exec(
			std::string title,
			const std::string &exe,
			const std::vector<std::string> &argv,
			UI::Shell &shell);
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
	void exec(
			std::string title,
			const std::string &exe,
			const std::vector<std::string> &argv);
	void ctl_kill(UI::Frame &ctx);
	void key_up(UI::Frame &ctx);
	void key_down(UI::Frame &ctx);
	void key_page_up(UI::Frame &ctx);
	void key_page_down(UI::Frame &ctx);
	void set_title(UI::Frame &ctx);
	unsigned maxscroll() const;
	std::unique_ptr<Subproc> _proc;
	std::unique_ptr<Log> _log;
	unsigned _scrollpos = 0;
	int _height = 0;
	int _width = 0;
};
} // namespace Console

#endif // CONSOLE_CONSOLE_H
