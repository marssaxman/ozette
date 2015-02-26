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

#include "dirtree.h"
#include <dirent.h>
#include <algorithm>
#include <sys/stat.h>
#include <queue>

DirTree::DirTree(std::string path):
	_path(path)
{
}

DirTree::DirTree(std::string dir, std::string name):
	_path(dir + "/" + name),
	_name(name)
{
	_casefold_name = _name;
	for_each(_casefold_name.begin(), _casefold_name.end(), [](char& in)
	{
		in = ::toupper(in);
	});
}

void DirTree::scan()
{
	struct stat st;
	if (stat(_path.c_str(), &st)) {
		_type = Type::None;
	} else if (S_ISDIR(st.st_mode)) {
		_type = Type::Directory;
		if (_mtime != st.st_mtime) {
			_items.clear();
			_iterated = false;
		}
	} else if (S_ISREG(st.st_mode)) {
		_type = Type::File;
	} else {
		_type = Type::Other;
	}
	_mtime = st.st_mtime;
	_scanned = true;
}

std::vector<DirTree> &DirTree::items()
{
	initcheck();
	if(!_iterated) iterate();
	return _items;
}

static bool entry_order(const DirTree &a, const DirTree &b)
{
	return a.casefold_name() < b.casefold_name();
}

void DirTree::iterate()
{
	_items.clear();
	_iterated = true;
	DIR *pdir = opendir(_path.c_str());
	if (!pdir) return;
	while (dirent *entry = readdir(pdir)) {
		if (entry->d_name[0] == '.') continue;
		_items.emplace_back(_path, entry->d_name);
	}
	closedir(pdir);
	std::sort(_items.begin(), _items.end(), entry_order);
}
