#include "dirtree.h"
#include <dirent.h>
#include <sys/stat.h>

DirTree::Directory::Directory(std::string path):
	Node(path)
{
	DIR *pdir = opendir(path.c_str());
	if (!pdir) return;
	while (dirent *entry = readdir(pdir)) {
		if (entry->d_name[0] == '.') continue;
		std::string name(entry->d_name);
		std::string subpath(path + "/" + name);
		switch (entry->d_type) {
		case DT_DIR: {
			auto sub = new Branch(name + "/", subpath);
			_items.emplace_back(sub);
		} break;
		case DT_REG: {
			auto sub = new File(name, subpath);
			_items.emplace_back(sub);
		} break;
		default: break;
		}
	}
	closedir(pdir);
}

DirTree::Root::Root(std::string path):
	Directory(path)
{
}

void DirTree::Root::render(ListForm::Builder &fields)
{
	for (auto &node: _items) {
		node->render(fields);
	}
}

DirTree::Branch::Branch(std::string name, std::string path):
	Directory(path),
	_name(name)
{
}

namespace {
class Indenter : public ListForm::Builder
{
public:
	Indenter(ListForm::Builder &dest): _dest(dest) {}
	virtual void entry(std::string text, std::function<void()> action) override
	{
		_dest.entry("    " + text, action);
	}
private:
	ListForm::Builder &_dest;
};
}

void DirTree::Branch::render(ListForm::Builder &fields)
{
	fields.entry(_name, [this](){_open = !_open;});
	if (!_open) return;
	Indenter subfields(fields);
	for (auto &node: _items) {
		node->render(subfields);
	}
}

DirTree::File::File(std::string name, std::string path):
	Node(path),
	_name(name)
{
}

void DirTree::File::render(ListForm::Builder &fields)
{
	std::string text = _name;
	struct stat st;
	if (!stat(_path.c_str(), &st)) {
		char buf[32];
		char *dt = ctime_r(&st.st_mtime, buf);
		if (dt) {
			text.push_back('\t');
			text += std::string(dt);
		}
	}
	fields.entry(text, [this](){});
}

