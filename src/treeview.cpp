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
	std::unique_ptr<Controller> sub(new RepoList(_host));
	_host.delegate(std::move(sub));
}
