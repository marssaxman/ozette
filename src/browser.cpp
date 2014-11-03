#include "browser.h"

Browser::Browser():
	_repos(*this)
{
}

void Browser::open_project(std::string path)
{
	_project.reset(new DirTree::Root(path));
}

void Browser::render(ListForm::Builder &lines)
{
	_repos.render(lines);
	if (_project) {
		_project->render(lines);
	}
}
