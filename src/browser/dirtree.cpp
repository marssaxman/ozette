#include "dirtree.h"
#include <dirent.h>
#include <sys/stat.h>

DirTree::DirTree(std::string path):
	_path(path)
{
}

DirTree::DirTree(std::string dir, std::string name):
	_path(dir + "/" + name),
	_name(name)
{
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
}
