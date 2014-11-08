#include "dirtree.h"
#include <dirent.h>
#include <algorithm>
#include <sys/stat.h>

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
			_scanned = false;
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
