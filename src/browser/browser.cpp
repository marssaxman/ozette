#include "browser.h"
#include "control.h"
#include "dialog.h"
#include <dirent.h>
#include <sys/stat.h>
#include <assert.h>

Browser::Browser():
	_homedir(getenv("HOME"))
{
	find_dirs();
}

void Browser::activate(UI::Frame &ctx)
{
	set_title(ctx);
	using namespace Control;
	Panel help = {{
		{Open, NewFile, 0, 0, 0, Find},
		{Quit, Save, Close, Directory, 0, Help} // Config/Settings
	}};
	ctx.set_help(help);
}

bool Browser::process(UI::Frame &ctx, int ch)
{
	bool more = true;
	switch (ch) {
		case Control::Directory: show_dirs(ctx); break;
		default: more = inherited::process(ctx, ch);
	}
	return more;
}

void Browser::show_dirs(UI::Frame &ctx)
{
	UI::Dialog::Picker dialog;
	dialog.prompt = "Change Directory";
	dialog.values = _dirs;
	dialog.commit = [this](UI::Frame &ctx, std::string path)
	{
		select_dir(ctx, path);
	};
	UI::Dialog::Show(dialog, ctx);
}

void Browser::set_title(UI::Frame &ctx)
{
	std::string title = "Lindi";
	if (_project.get()) {
		title += ": " + _project->path();
	}
	ctx.set_title(title);
}

void Browser::render(ListForm::Builder &lines)
{
	if (_project) {
		_project->render(lines);
	}
}

void Browser::select_dir(UI::Frame &ctx, std::string path)
{
	_project.reset(new DirTree::Root(path));
	set_title(ctx);
	mark_dirty();
	ctx.repaint();
}

void Browser::find_dirs()
{
	_dirs.clear();
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
			_dirs.push_back(path);
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


