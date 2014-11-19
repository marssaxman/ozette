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

#ifndef LINDI_H
#define LINDI_H

#include "app.h"
#include "shell.h"
#include "browser.h"
#include <string>
#include <vector>
#include <map>

class Lindi : public App
{
public:
	Lindi();
	virtual std::string current_dir() const override;
	virtual std::string display_path(std::string path) const override;
	virtual void edit_file(std::string path) override;
	virtual void rename_file(std::string from, std::string to) override;
	virtual void close_file(std::string path) override;
	virtual void set_clipboard(std::string text) override;
	virtual std::string get_clipboard() override;
	virtual void get_config(std::string name, std::vector<std::string> &lines) override;
	virtual void set_config(std::string name, const std::vector<std::string> &lines) override;

	void run();
private:
	void show_browser();
	void change_directory();
	void new_file();
	void open_file();
	void show_help();
	void execute();
	int fix_control_quirks(int ch);
	static void set_mru(std::string path, std::vector<std::string> &mru);
	std::string canonical_abspath(std::string path);

	UI::Shell _shell;
	std::string _home_dir;
	std::string _current_dir;
	std::string _config_dir;
	std::map<std::string, UI::Window*> _editors;
	std::string _clipboard;
	bool _done = false;
	std::vector<std::string> _recent_dirs;
};

#endif	//LINDI_H
