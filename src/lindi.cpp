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
#include "dialog.h"
#include "help.h"
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

std::string Lindi::display_path(std::string path) const
{
	if (path.substr(0, _current_dir.size()) == _current_dir) {
		return path.substr(1 + _current_dir.size());
	}
	if (path.substr(0, _home_dir.size()) == _home_dir) {
		return "~" + path.substr(_home_dir.size());
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
	std::unique_ptr<UI::View> ed(new Editor::View(path));
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

void Lindi::run()
{
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
	Browser::open(_current_dir, _shell);
}

void Lindi::change_directory()
{
	std::string prompt = "Change Directory";
	get_config("recent_dirs", _recent_dirs);
	auto options = _recent_dirs;
	set_mru(_current_dir, options);
	auto commit = [this](UI::Frame &ctx, std::string path)
	{
		_current_dir = path;
		Browser::change_directory(path);
		set_mru(path, _recent_dirs);
		set_config("recent_dirs", _recent_dirs);
	};
	auto dialog = new UI::Dialog::Pick(prompt, options, commit);
	std::unique_ptr<UI::View> dptr(dialog);
	_shell.active()->show_dialog(std::move(dptr));
}

void Lindi::new_file()
{
	std::unique_ptr<UI::View> ed(new Editor::View());
	_editors[canonical_abspath("")] = _shell.open_window(std::move(ed));
}

void Lindi::open_file()
{
	std::string prompt = "Open";
	std::vector<std::string> options;
	auto commit = [this](UI::Frame &ctx, std::string path)
	{
		if (path.empty()) return;
		edit_file(path);
	};
	auto dialog = new UI::Dialog::Pick(prompt, options, commit);
	std::unique_ptr<UI::View> dptr(dialog);
	_shell.active()->show_dialog(std::move(dptr));
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
	Editor::Document doc;
	doc.View(helptext);
	std::unique_ptr<UI::View> ed(new Editor::View(help_key, std::move(doc)));
	_editors[abs_help] = _shell.open_window(std::move(ed));
}

void Lindi::execute()
{
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


