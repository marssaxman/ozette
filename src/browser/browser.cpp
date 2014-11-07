#include "browser.h"
#include "control.h"
#include "dialog.h"
#include <dirent.h>
#include <sys/stat.h>
#include <assert.h>

Browser::Browser()
{
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

void Browser::view(std::string path)
{
	_tree.reset(new DirTree::Root(path));
	mark_dirty();
}

void Browser::set_title(UI::Frame &ctx)
{
	std::string title = "Lindi";
	if (_tree.get()) {
		title += ": " + _tree->path();
	}
	ctx.set_title(title);
}

void Browser::render(ListForm::Builder &lines)
{
	if (_tree) {
		_tree->render(lines);
	}
}

