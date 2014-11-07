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
		{Open, NewFile, 0, 0, Find, GoTo},
		{Quit, Save, Close, Projects, 0, 0} // Projects, Config, Help
	}};
	ctx.set_help(help);
}

bool Browser::process(Context &ctx, int ch)
{
	bool more = true;
	switch (ch) {
		case Control::Projects: show_projects(); break;
		default: more = inherited::process(ctx, ch);
	}
	return more;
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
