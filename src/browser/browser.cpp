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
		case Control::Projects: show_projects(ctx); break;
		default: more = inherited::process(ctx, ch);
	}
	return more;
}

void Browser::show_projects(Context &ctx)
{
	class Action : public UI::Dialog::Controller
	{
	public:
		Action(std::string path, Browser &browser, ProjectList &list):
			_path(path), _browser(browser), _list(list) {}
		virtual void open(UI::Dialog::State &state)
		{
			state.prompt = "Project";
			state.suggestions.clear();
			for (auto &repo: _list._repos) {
				state.suggestions.push_back(repo.path);
			}
			state.value = _path;
		}
		virtual void commit(std::string path)
		{
			_browser.open_project(path);
		}
	private:
		std::string _path;
		Browser &_browser;
		ProjectList &_list;
	};
	std::string current;
	if (_project.get()) current = _project->path();
	auto action = new Action(current, *this, _repos);
	std::unique_ptr<UI::Dialog::Controller> actionptr(action);
	ctx.show_dialog(std::move(actionptr));
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
