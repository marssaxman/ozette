// ozette
// Copyright (C) 2014-2025 Mars J. Saxman
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

#ifndef APP_OZETTE_H
#define APP_OZETTE_H

#include <map>
#include <string>
#include <vector>
#include "app/controller.h"
#include "browser/browser.h"
#include "editor/editor.h"
#include "ui/shell.h"

class Ozette : public Controller {
public:
	Ozette();
	virtual void change_dir(std::string path) override;
	virtual void edit_file(std::string path) override;
	virtual void rename_file(std::string from, std::string to) override;
	virtual void close_file(std::string path) override;
	virtual void find_in_file(std::string path, Editor::line_t index) override;
	virtual void set_clipboard(std::string text) override;
	virtual std::string get_clipboard() override;
	virtual void cache_read(std::string name, std::vector<std::string> &lines) override;
	virtual void cache_write(std::string name, const std::vector<std::string> &lines) override;
	virtual void search(Search::spec query) override;
	virtual void save_session() override;
	virtual void load_session() override;
	void run();
	void sig_io();
private:
	struct editor {
		UI::Window *window;
		Editor::View *view;
	};
	void show_browser();
	void change_directory();
	void new_file();
	void open_file();
	void show_help();
	void execute();
	void build();
	void search();
	int fix_control_quirks(int ch);
	void exec(std::string command);
	editor open_editor(std::string path);
	UI::Shell _shell;
	std::string _home_dir;
	std::string _current_dir;
    std::string _cache_dir;
	std::map<std::string, editor> _editors;
	std::string _clipboard;
	bool _done = false;
};

#endif //APP_OZETTE_H

