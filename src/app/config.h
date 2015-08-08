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

#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <string>
#include <memory>
#include "inih/INIReader.h"

class Config
{
public:
	Config(std::string confdir, std::string workingdir);
	void change_directory(std::string workingdir);
	std::string get(std::string key, std::string def_val) const;
	int get_int(std::string key, int def_val) const;
	bool get_bool(std::string key, bool def_val) const;
	std::string get(std::string group, std::string key, std::string val) const;
	int get_int(std::string group, std::string key, int val) const;
	bool get_bool(std::string group, std::string key, bool val) const;
private:
	std::unique_ptr<INIReader> _user;
	std::unique_ptr<INIReader> _dir;
};

#endif //APP_CONFIG_H
