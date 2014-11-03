#include "treeview.h"
#include "repolist.h"
#include <dirent.h>
#include <sys/stat.h>

TreeView::TreeView(Browser &host, std::string path):
	_host(host),
	_path(path),
	_tree(path)
{
}

void TreeView::render(Builder &fields)
{
	fields.entry("Switch Repository", [this](){switchrepo();});
	fields.blank();
	_tree.render(fields);
}

void TreeView::switchrepo()
{
	_host.close_project();
}
