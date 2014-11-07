#include "browser.h"
#include "control.h"
#include "dialog.h"
#include <dirent.h>
#include <sys/stat.h>

Browser::Browser():
	_homedir(getenv("HOME"))
{
	find_projects();
}

void Browser::open(UI::Frame &ctx)
{
	ctx.set_title("Lindi");
	using namespace Control;
	Panel help = {{
		{Open, NewFile, 0, 0, Find, GoTo},
		{Quit, Save, Close, Projects, 0, Help} // Config/Settings
	}};
	ctx.set_help(help);
}

bool Browser::process(UI::Frame &ctx, int ch)
{
	bool more = true;
	switch (ch) {
		case Control::Projects: show_projects(ctx); break;
		default: more = inherited::process(ctx, ch);
	}
	return more;
}

namespace {
class SetProject : public UI::Dialog::Action
{
public:
	SetProject(Browser *browser): _browser(browser) {}
	virtual void commit(UI::Frame &ctx, std::string path) override
	{
		_browser->set_project(ctx, path);
	}
	Browser *_browser;
};
} // namespace

void Browser::show_projects(UI::Frame &ctx)
{
	auto action = new SetProject(this);
	std::unique_ptr<UI::Dialog::Action> actionptr(action);
	auto dialog = new UI::Dialog("Select Project", _projects, std::move(actionptr));
	std::unique_ptr<UI::Dialog> dialogptr(dialog);
	ctx.show_dialog(std::move(dialogptr));
}

void Browser::render(ListForm::Builder &lines)
{
	if (_project) {
		_project->render(lines);
	}
}

void Browser::set_project(UI::Frame &ctx, std::string path)
{
	_project.reset(new DirTree::Root(path));
	ctx.set_title("Lindi: " + path);
	mark_dirty();
	ctx.repaint();
}

void Browser::find_projects()
{
	_projects.clear();
	// Iterate through the directories in the homedir looking for things
	// which might be program directories. Clues are things like version
	// control directories or a Makefile.
	DIR *pdir = opendir(_homedir.c_str());
	if (!pdir) return;
	while (dirent *entry = readdir(pdir)) {
		// We are looking only for directories.
		if (entry->d_type != DT_DIR) continue;
		// Look for specific items in the directory that might be
		// there if this were a program root.
		std::string path = _homedir + "/" + entry->d_name;
		bool include = false;
		include |= dir_exists(path + "/.git");
		include |= dir_exists(path + "/.svn");
		include |= file_exists(path + "/Makefile");
		if (include) {
			_projects.push_back(path);
		}
	}
	closedir(pdir);
}

bool Browser::dir_exists(std::string path)
{
        struct stat sb;
        return (stat(path.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode));
}

bool Browser::file_exists(std::string path)
{
        struct stat sb;
        return (stat(path.c_str(), &sb) == 0 && S_ISREG(sb.st_mode));
}


