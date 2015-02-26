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

#include "config.h"
#include "INIReader.h"

Config::Config(std::string confdir, std::string workingdir):
	_user(new INIReader(confdir + "/config")),
	_dir(new INIReader(workingdir + "/.ozette/config"))
{
}

void Config::change_directory(std::string workingdir)
{
	_dir.reset(new INIReader(workingdir + "/.ozette/config"));
}

std::string Config::get(std::string key, std::string val) const
{
	return get("", key, val);
}

int Config::get_int(std::string key, int val) const
{
	return get_int("", key, val);
}

bool Config::get_bool(std::string key, bool val) const
{
	return get_bool("", key, val);
}

std::string Config::get(
		std::string group, std::string key, std::string val) const
{
	if (!_user->ParseError()) {
		val = _user->Get(group, key, val);
	}
	if (!_dir->ParseError()) {
		val = _dir->Get(group, key, val);
	}
	return val;
}

int Config::get_int(std::string group, std::string key, int val) const
{
	if (!_user->ParseError()) {
		val = (int)_user->GetInteger(group, key, (long)val);
	}
	if (!_dir->ParseError()) {
		val = (int)_dir->GetInteger(group, key, (long)val);
	}
	return val;
}

bool Config::get_bool(std::string group, std::string key, bool val) const
{
	if (!_user->ParseError()) {
		val = _user->GetBoolean(group, key, val);
	}
	if (!_dir->ParseError()) {
		val = _dir->GetBoolean(group, key, val);
	}
	return val;
}

