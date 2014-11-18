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

#ifndef APP_H
#define APP_H

#include <string>
#include <vector>

// Abstract interface for centralized application actions.
class App
{
public:
	virtual ~App() = default;
	virtual std::string current_dir() const = 0;
	virtual std::string display_path(std::string path) const = 0;
	virtual void edit_file(std::string path) = 0;
	virtual void rename_file(std::string from, std::string to) = 0;
	virtual void close_file(std::string path) = 0;
	virtual void set_clipboard(std::string text) = 0;
	virtual std::string get_clipboard() = 0;
	virtual void get_config(std::string name, std::vector<std::string> &lines) = 0;
	virtual void set_config(std::string name, const std::vector<std::string> &lines) = 0;
};

#endif	//APP_H
