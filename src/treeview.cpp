#include "treeview.h"
#include "repolist.h"
#include <dirent.h>

TreeView::TreeView(Browser &host, std::string path):
	_host(host),
	_path(path),
	_dir(path)
{
}

void TreeView::render(Builder &fields)
{
	fields.entry("Switch Repository", [this](){switchrepo();});
	fields.blank();
	_dir.render(fields);
}

void TreeView::switchrepo()
{
	std::unique_ptr<Controller> sub(new RepoList(_host));
	_host.delegate(std::move(sub));
}

TreeView::Directory::Directory(std::string path):
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

TreeView::Root::Root(std::string path):
	Directory(path)
{
}

void TreeView::Root::render(Builder &fields)
{
	for (auto &node: _items) {
		node->render(fields);
	}
}

TreeView::Branch::Branch(std::string name, std::string path):
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

void TreeView::Branch::render(Builder &fields)
{
	fields.entry(_name, [this](){_open = !_open;});
	if (!_open) return;
	Indenter subfields(fields);
	for (auto &node: _items) {
		node->render(subfields);
	}
}

TreeView::File::File(std::string name, std::string path):
	Node(path),
	_name(name)
{
}

void TreeView::File::render(Builder &fields)
{
	fields.entry(_name, [this](){});
}

