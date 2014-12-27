//
// lindi
// Copyright (C) 2014 Mars J. Saxman
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//

#include "lindi.h"
#include "browser.h"
#include "editor.h"
#include "console.h"
#include "control.h"
#include "picker.h"
#include "help.h"
#include <unistd.h>
#include <fstream>
#include <sys/stat.h>

Lindi::Lindi():
	_shell(*this),
	_home_dir(getenv("HOME")),
	_config_dir(_home_dir + "/.lindi"),
	_config(_config_dir, ".")
{
	char *cwd = getcwd(NULL, 0);
	if (cwd) {
		_current_dir = cwd;
		free(cwd);
	} else {
		_current_dir = _home_dir;
	}
	_config.change_directory(_current_dir);
}

std::string Lindi::current_dir() const
{
	return _current_dir;
}

std::string Lindi::display_path(std::string path) const
{
	size_t cwdsize = _current_dir.size();
	if (path.size() > cwdsize && path.substr(0, cwdsize) == _current_dir) {
		return path.substr(1 + cwdsize);
	}
	size_t homesize = _home_dir.size();
	if (path.substr(0, homesize) == _home_dir) {
		return "~" + path.substr(homesize);
	}
	return path;
}

void Lindi::edit_file(std::string path)
{
	// If we already have this file open, bring it forward.
	path = canonical_abspath(path);
	auto existing = _editors.find(path);
	if (existing != _editors.end()) {
		_shell.make_active(existing->second);
		return;
	}
	// We don't have an editor for this file, so we should create one.
	std::unique_ptr<UI::View> ed(new Editor::View(path, _config));
	UI::Window *win = _shell.open_window(std::move(ed));
	_editors[path] = win;
}

void Lindi::rename_file(std::string from, std::string to)
{
	// Somebody has moved or renamed a file. If there is an editor
	// open for it, update our editor map.
	auto existing = _editors.find(canonical_abspath(from));
	if (existing == _editors.end()) return;
	auto window = existing->second;
	_editors.erase(existing);
	_editors[canonical_abspath(to)] = window;
}

void Lindi::close_file(std::string path)
{
	auto iter = _editors.find(canonical_abspath(path));
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

void Lindi::cache_read(std::string name, std::vector<std::string> &lines)
{
	lines.clear();
	std::string str;
	std::ifstream file(_config_dir + "/" + name);
	while (std::getline(file, str)) {
		lines.push_back(str);
	}
	file.close();
}

void Lindi::cache_write(std::string name, const std::vector<std::string> &lines)
{
	// if the lindi directory doesn't exist yet, create it
	struct stat st;
	if (stat(_config_dir.c_str(), &st)) {
		mkdir(_config_dir.c_str(), S_IRWXU);
	}
	std::ofstream file(_config_dir + "/" + name, std::ios::trunc);
	for (auto &line: lines) {
		file << line << std::endl;
	}
	file.close();
}

Config &Lindi::config()
{
	return _config;
}

void Lindi::exec(std::string title, std::string exe, const std::vector<std::string> &argv)
{
	Console::View::exec(title, exe, argv, _shell);
}

void Lindi::run()
{
	if (_editors.empty()) show_browser();
	timeout(20);
	do {
		update_panels();
		doupdate();
		int ch = fix_control_quirks(getch());
		switch (ch) {
			case Control::UpArrow: show_browser(); break;
			case Control::NewFile: new_file(); break;
			case Control::Open: open_file(); break;
			case Control::Directory: change_directory(); break;
			case Control::Help: show_help(); break;
			case Control::Execute: execute(); break;
			default: _done |= !_shell.process(ch);
		}
	} while (!_done);
}

void Lindi::show_browser()
{
	Browser::View::open(_current_dir, _shell);
}

void Lindi::change_directory()
{
	show_browser();
	std::string prompt = "Change Directory";
	// Get the cached list of directories we have opened. Strip out any dirs
	// which no longer exist, then make a temporary copy of the list in which
	// the current directory is most recent. Format the paths in the display
	// list using the tilde ~ in place of the home dir prefix, since that reads
	// better, though we store full paths in the cache.
	cache_read("recent_dirs", _recent_dirs);
	for (size_t i = _recent_dirs.size(); i-- > 0;) {
		std::string path = _recent_dirs[i];
		struct stat st;
		if (stat(path.c_str(), &st)) {
			_recent_dirs.erase(_recent_dirs.begin() + i);
		}
	}
	auto options = _recent_dirs;
	set_mru(_current_dir, options);
	for (auto &path: options) {
		if (path.substr(0, _home_dir.size()) == _home_dir) {
			path = "~" + path.substr(_home_dir.size());
		}
	}
	auto commit = [this](UI::Frame &ctx, std::string path)
	{
		path = canonical_abspath(path);
		int result = chdir(path.c_str());
		if (0 != result) {
			int code = errno;
			ctx.show_result("Can't chdir: errno = " + std::to_string(code));
			return;
		}
		_current_dir = path;
		_config.change_directory(path);
		Browser::View::change_directory(path);
		set_mru(path, _recent_dirs);
		cache_write("recent_dirs", _recent_dirs);
	};
	auto dialog = new Browser::Picker(prompt, options, commit);
	std::unique_ptr<UI::View> dptr(dialog);
	_shell.active()->show_dialog(std::move(dptr));
}

void Lindi::new_file()
{
	std::unique_ptr<UI::View> ed(new Editor::View(_config));
	_editors[canonical_abspath("")] = _shell.open_window(std::move(ed));
}

void Lindi::open_file()
{
	show_browser();
	_done |= _shell.process(Control::Open);
}

void Lindi::show_help()
{
	static const std::string help_key = " Help ";
	static const std::string abs_help = canonical_abspath(help_key);
	auto existing = _editors.find(abs_help);
	if (existing != _editors.end()) {
		_shell.make_active(existing->second);
		return;
	}
	std::string helptext((const char*)HELP, HELP_len);
	Editor::Document doc(_config);
	doc.View(helptext);
	std::unique_ptr<UI::View> ed(new Editor::View(help_key, std::move(doc)));
	_editors[abs_help] = _shell.open_window(std::move(ed));
}

void Lindi::execute()
{
	show_browser();
	std::string prompt = "exec";
	auto commit = [this](UI::Frame &ctx, std::string cmd)
	{
		Console::View::exec(cmd, _shell);
	};
	auto dialog = new UI::Dialog::Command(prompt, commit);
	std::unique_ptr<UI::View> dptr(dialog);
	_shell.active()->show_dialog(std::move(dptr));
}

int Lindi::fix_control_quirks(int ch)
{
	// Terminals are weird and control keys are complicated.
	switch (ch) {

	// When the user presses the backspace key, some terminals send us
	// ASCII BS, others send ASCII DEL, and others send something that
	// ncurses interprets as KEY_BACKSPACE. We will stick with ASCII BS.
	case KEY_BACKSPACE:
	case 0x7F: return Control::Backspace;

	// Different terminals send us different codes for control-left and
	// control-right arrow. Some terminals send us a different code for
	// control-left and control-right arrow when the shift key is held
	// down, while others don't distinguish.
	case 0x21D:
	case 0x220: return Control::LeftArrow;
	case 0x22C:
	case 0x22F: return Control::RightArrow;
	case 0x20C: return Control::DownArrow;

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

std::string Lindi::canonical_abspath(std::string path)
{
	// Canonicalize this path and expand it as necessary to produce
	// a full path relative to the filesystem root.
	if (path.empty()) return _current_dir;
	std::string out;
	size_t offset = 0;
	if (path[0] == '/') {
		offset = 1;
	} else {
		out = _current_dir;
	}
	while (offset != std::string::npos) {
		size_t nextseg = path.find_first_of('/', offset);
		std::string seg;
		if (nextseg == std::string::npos) {
			seg = path.substr(offset);
			offset = nextseg;
		} else {
			seg = path.substr(offset, nextseg - offset);
			offset = nextseg + 1;
		}
		if (seg.empty()) continue;
		if (seg == ".") continue;
		if (seg == "~") {
			out = _home_dir;
			continue;
		}
		if (seg == "..") {
			size_t trunc = out.find_last_of('/');
			if (trunc == std::string::npos) {
				trunc = 0;
			}
			out.resize(trunc);
			continue;
		}
		out += "/" + seg;
	}
	return out;
}


