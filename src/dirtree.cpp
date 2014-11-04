#include "dirtree.h"
#include <dirent.h>
#include <sys/stat.h>

DirTree::Node::Node(std::string path, unsigned indent):
	_path(path),
	_indent(indent)
{
}

namespace {
class NodeField : public ListForm::Field
{
protected:
	void indent(WINDOW *view, unsigned tabs, int widget, size_t &width)
	{
		while (tabs-- > 0) {
			emitrep(view, ' ', 4, width);
		}
		wattron(view, A_DIM);
		emitch(view, widget, width);
		wattroff(view, A_DIM);
		emitch(view, ' ', width);
	}
};
} // namespace

DirTree::Directory::Directory(std::string path, unsigned indent, unsigned subindent):
	Node(path, indent)
{
	DIR *pdir = opendir(path.c_str());
	if (!pdir) return;
	while (dirent *entry = readdir(pdir)) {
		if (entry->d_name[0] == '.') continue;
		std::string name(entry->d_name);
		std::string subpath(path + "/" + name);
		switch (entry->d_type) {
		case DT_DIR: {
			auto sub = new Branch(name + "/", subpath, subindent);
			_items.emplace_back(sub);
		} break;
		case DT_REG: {
			auto sub = new File(name, subpath, subindent);
			_items.emplace_back(sub);
		} break;
		default: break;
		}
	}
	closedir(pdir);
}

DirTree::Root::Root(std::string path):
	Directory(path, 0, 0)
{
}

void DirTree::Root::render(ListForm::Builder &fields)
{
	for (auto &node: _items) {
		node->render(fields);
	}
}

DirTree::Branch::Branch(std::string name, std::string path, unsigned indent):
	Directory(path, indent, indent + 1),
	_name(name)
{
}

namespace {
class BranchField : public NodeField
{
public:
	BranchField(DirTree::Branch &branch): _branch(branch) {}
	virtual bool invoke() override
	{
		_branch.toggle();
		return true;
	}
	virtual void paint(WINDOW *view, size_t width) override
	{
		int widget = _branch.is_open() ? '-' : '+';
		indent(view, _branch.indent(), widget, width);
		waddnstr(view, _branch.name().c_str(), width);
	}
private:
	DirTree::Branch &_branch;
};
}

void DirTree::Branch::render(ListForm::Builder &fields)
{
	std::unique_ptr<ListForm::Field> field(new BranchField(*this));
	fields.add(std::move(field));
	if (!_open) return;
	for (auto &node: _items) {
		node->render(fields);
	}
}

DirTree::File::File(std::string name, std::string path, unsigned indent):
	Node(path, indent),
	_name(name)
{
}

namespace {
class FileField : public NodeField
{
public:
	FileField(DirTree::File &file): _file(file)
	{
		struct stat st;
		std::string path = _file.path();
		if (!stat(path.c_str(), &st)) {
			char buf[32];
			_modtime = ctime_r(&st.st_mtime, buf);
		}
	}
	virtual bool invoke(App &app) override
	{
		app.edit_file(_file.path());
		return false;
	}
	virtual void paint(WINDOW *view, size_t width)
	{
		indent(view, _file.indent(), ' ', width);
		emitstr(view, _file.name(), width);
		size_t datelen = _modtime.size();
		if (datelen > width) return;
		emitrep(view, ' ', width - datelen, width);
		wattron(view, A_DIM);
		emitstr(view, _modtime, width);
		wattroff(view, A_DIM);
	}
private:
	DirTree::File &_file;
	std::string _modtime;
};
}

void DirTree::File::render(ListForm::Builder &fields)
{
	std::unique_ptr<ListForm::Field> field(new FileField(*this));
	fields.add(std::move(field));
}

