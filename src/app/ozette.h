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

#ifndef APP_OZETTE_H
#define APP_OZETTE_H

#include "app/controller.h"
#include "ui/shell.h"
#include "browser/browser.h"
#include "editor/editor.h"
#include <string>
#include <vector>
#include <map>

class Ozette : public Controller
{
public:
	Ozette();
	virtual std::string current_dir() const override;
	virtual void change_dir(std::string path) override;
	virtual std::string display_path(std::string path) const override;
	virtual void edit_file(std::string path) override;
	virtual void rename_file(std::string from, std::string to) override;
	virtual void close_file(std::string path) override;
	virtual void find_in_file(std::string path, Editor::line_t index);
	virtual void set_clipboard(std::string text) override;
	virtual std::string get_clipboard() override;
	virtual void cache_read(std::string name, std::vector<std::string> &lines) override;
	virtual void cache_write(std::string name, const std::vector<std::string> &lines) override;
	virtual Config &config() override;
	void find(std::string regex, std::string tree) override;

	void run();
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
	int fix_control_quirks(int ch);
	static void set_mru(std::string path, std::vector<std::string> &mru);
	std::string canonical_abspath(std::string path);
	void exec(std::string command);
	editor open_editor(std::string path);

	UI::Shell _shell;
	std::string _home_dir;
	std::string _current_dir;
	std::string _config_dir;
	Config _config;
	std::map<std::string, editor> _editors;
	std::string _clipboard;
	bool _done = false;
	std::vector<std::string> _recent_dirs;
};

#endif //APP_OZETTE_H

