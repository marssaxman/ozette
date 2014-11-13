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

#ifndef BROWSER_DIRTREE_H
#define BROWSER_DIRTREE_H

#include <vector>
#include <string>

class DirTree
{
public:
	DirTree(std::string path);
	DirTree(std::string dir, std::string name);
	void scan();
	std::string path() const { return _path; }
	std::string name() const { return _name; }
	std::string casefold_name() const { return _casefold_name; }
	enum class Type {
		Directory,
		File,
		Other,
		None
	};
	bool is_directory() { return type() == Type::Directory; }
	bool is_file() { return type() == Type::File; }
	Type type() { initcheck(); return _type; }
	time_t mtime() { initcheck(); return _mtime; }
	std::vector<DirTree> &items();
private:
	void initcheck() { if (!_scanned) scan(); }
	void iterate();
	std::string _path;
	std::string _name;
	std::string _casefold_name;
	bool _scanned = false;
	Type _type = Type::None;
	time_t _mtime = 0;
	bool _iterated = false;
	std::vector<DirTree> _items;
};

#endif	//BROWSER_DIRTREE_H
