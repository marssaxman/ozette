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

#ifndef BROWSER_BROWSER_H
#define BROWSER_BROWSER_H

#include "view.h"
#include "dirtree.h"
#include "shell.h"
#include <set>

namespace Browser {
class View : public UI::View
{
public:
	static void change_directory(std::string path);
	static void open(std::string path, UI::Shell &shell);
	virtual void activate(UI::Frame &ctx) override;
	virtual void deactivate(UI::Frame &ctx) override;
	virtual bool process(UI::Frame &ctx, int ch) override;
	virtual bool poll(UI::Frame &ctx) override;
	virtual void set_help(UI::HelpBar::Panel &panel) override;
	void view(std::string path);
	virtual Priority priority() const { return Priority::Primary; }
protected:
	void check_rebuild(UI::Frame &ctx);
	virtual void paint_into(WINDOW *view, bool active) override;
private:
	View(std::string path);
	~View() { _instance = nullptr; }
	static View *_instance;
	UI::Window *_window = nullptr;
	struct row_t {
		unsigned indent;
		bool expanded;
		DirTree *entry;
	};
	void paint_row(WINDOW *view, int vpos, row_t &display, int width);
	void ctl_find(UI::Frame &ctx);
	void key_return(UI::Frame &ctx);
	void key_up(UI::Frame &ctx);
	void key_down(UI::Frame &ctx);
	void key_page_up(UI::Frame &ctx);
	void key_page_down(UI::Frame &ctx);
	void key_left(UI::Frame &ctx);
	void key_right(UI::Frame &ctx);
	void key_space(UI::Frame &ctx);
	void key_tab(UI::Frame &ctx);
	void key_char(UI::Frame &ctx, char ch);
	void clear_filter(UI::Frame &ctx);
	bool matches_filter(size_t index);
	void set_title(UI::Frame &ctx);
	void build_list();
	void toggle(UI::Frame &ctx);
	void edit_file(UI::Frame &ctx);
	size_t insert_rows(size_t index, unsigned indent, DirTree *entry);
	void remove_rows(size_t index, unsigned indent);
	DirTree *sel_entry() { return _list[_selection].entry; }
	DirTree _tree;
	std::vector<row_t> _list;
	std::set<std::string> _expanded_items;
	size_t _selection = 0;
	size_t _scrollpos = 0;
	int _height = 0;
	bool _rebuild_list = true;
	std::string _name_filter;
	time_t _name_filter_time = 0;
};
} //namespace Browser

#endif // BROWSER_BROWSER_H
