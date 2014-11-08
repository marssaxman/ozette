#include "lindi.h"
#include "browser.h"
#include "editor.h"
#include "control.h"
#include <unistd.h>
#include <fstream>
#include <sys/stat.h>

Lindi::Lindi():
	_shell(*this),
	_home_dir(getenv("HOME")),
	_config_dir(_home_dir + "/.lindi")
{
	char *cwd = get_current_dir_name();
	if (cwd) {
		_current_dir = cwd;
		free(cwd);
	} else {
		_current_dir = _home_dir;
	}
	Browser::open(_current_dir, _shell);
}

std::string Lindi::current_dir() const
{
	return _current_dir;
}

void Lindi::edit_file(std::string path)
{
	// If we already have this file open, bring it forward.
	auto existing = _editors.find(path);
	if (existing != _editors.end()) {
		_shell.make_active(existing->second);
		return;
	}
	// We don't have an editor for this file, so we should create one.
	std::unique_ptr<UI::Controller> ed(new Editor::Controller(path));
	UI::Window *win = _shell.open_window(std::move(ed));
	_editors[path] = win;
}

void Lindi::close_file(std::string path)
{
	auto iter = _editors.find(path);
	if (iter != _editors.end()) {
		_shell.close_window(iter->second);
		_editors.erase(iter);
	}
}

void Lindi::set_clipboard(std::string text)
{
	_clipboard = text;
}

std::string Lindi::get_clipboard()
{
	return _clipboard;
}

void Lindi::get_config(std::string name, std::vector<std::string> &lines)
{
	lines.clear();
	std::string str;
	std::ifstream file(_config_dir + "/" + name);
	while (std::getline(file, str)) {
		lines.push_back(str);
	}
	file.close();
}

void Lindi::set_config(std::string name, const std::vector<std::string> &lines)
{
	// if the lindi prefs directory doesn't exist yet, create it
	mkdir(_config_dir.c_str(), S_IRWXU);
	std::ofstream file(_config_dir + "/" + name, std::ios::trunc);
	for (auto &line: lines) {
		file << line << std::endl;
	}
	file.close();
}

void Lindi::run()
{
	timeout(20);
	do {
		update_panels();
		doupdate();
		int ch = fix_control_quirks(getch());
		switch (ch) {
			case Control::Quit: quit(); break;
			case Control::NewFile: new_file(); break;
			case Control::Directory: change_directory(); break;
			default: _done |= !_shell.process(ch);
		}
	} while (!_done);
}

void Lindi::change_directory()
{
	UI::Dialog::Picker dialog;
	dialog.prompt = "Change Directory";
	get_config("recent_dirs", dialog.values);
	set_mru(_current_dir, dialog.values);
	dialog.commit = [this](UI::Frame &ctx, std::string path)
	{
		_current_dir = path;
		Browser::change_directory(path);
		set_mru(path, _recent_dirs);
		set_config("recent_dirs", _recent_dirs);
	};
	UI::Dialog::Show(dialog, *_shell.active());
}

void Lindi::new_file()
{
	std::unique_ptr<UI::Controller> ed(new Editor::Controller());
	_shell.open_window(std::move(ed));
}

int Lindi::fix_control_quirks(int ch)
{
	// Terminals are weird and control keys are complicated.

	switch (ch) {
	case KEY_BACKSPACE:
	// ncurses defines KEY_BACKSPACE, but the actual backspace key
	// produces 0x7F, good old ASCII DEL. That's fine, but the way
	// you get KEY_BACKSPACE is by pressing ctrl-H. This makes a
	// certain amount of sense as ^H corresponds to 0x08 aka the
	// ASCII BS code... but it's an unhelpful bit of help as there
	// is no other situation I can find where ncurses will send us
	// 0x08. (The forward-delete key comes in as KEY_DC.) It is
	// much more convenient for us to keep all the control keys in
	// their traditional places, so we will change KEY_BACKSPACE back
	// to 0x08, with the understanding that it always means ctrl-H.
	return 0x08;

	default:
	return ch;
	}
}

void Lindi::set_mru(std::string path, std::vector<std::string> &mru)
{
	// make this item the front of the list
	// if there are other instances, remove them
	mru.insert(mru.begin(), path);
	for (size_t i = 1; i < mru.size();) {
		if (mru[i] == path) {
			mru.erase(mru.begin() + i);
		} else {
			++i;
		}
	}
}
