#include "browser.h"
#include "control.h"

Browser::Browser():
	_repos(*this)
{
}

void Browser::open(Context &ctx)
{
	ctx.set_title("Lindi");
	using namespace Control;
	Panel help = {{
		{Open, 0, 0, 0, Find, GoTo},
		{Quit, Save, Close, 0, 0, 0} // Projects, Config, Help
	}};
	ctx.set_help(help);
}

void Browser::show_projects()
{
	_repos.show_projects();
	mark_dirty();
}

void Browser::open_project(std::string path)
{
	_project.reset(new DirTree::Root(path));
}

void Browser::render(ListForm::Builder &lines)
{
	_repos.render(lines);
	lines.blank();
	if (_project) {
		_project->render(lines);
		lines.blank();
	}
}
