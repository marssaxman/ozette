// ozette
// Copyright (C) 2014-2023 Mars J. Saxman
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

#include <assert.h>
#include <atomic>
#include <fstream>
#include <cstdlib>
#include <sys/stat.h>
#include <unistd.h>
#include "app/control.h"
#include "app/ozette.h"
#include "app/path.h"
#include "console/console.h"
#include "help/view.h"
#include "search/dialog.h"
#include "search/search.h"

std::atomic_bool sig_io_flag;

Ozette::Ozette():
		_shell(*this),
		_home_dir(std::getenv("HOME")) {
	char *cwd = getcwd(NULL, 0);
	if (cwd) {
		_current_dir = cwd;
		free(cwd);
	} else {
		_current_dir = _home_dir;
	}
	if (const char *cache = std::getenv("XDG_CACHE_HOME")) {
		_cache_dir = std::string(cache);
	} else {
		_cache_dir = _home_dir + "/.cache/ozette";
	}
}

void Ozette::change_dir(std::string path) {
	path = Path::absolute(path);
	int result = chdir(path.c_str());
	if (0 != result) {
		int code = errno;
		std::string message = "Can't chdir: errno = " + std::to_string(code);
		UI::Frame *ctx = _shell.active();
		ctx->show_result(message);
		return;
	}
	_current_dir = path;
	Browser::View::change_directory(path);
}

void Ozette::edit_file(std::string path) {
	struct stat st;
	if (0 == stat(path.c_str(), &st) && S_ISDIR(st.st_mode)) {
		change_dir(path);
	} else {
		open_editor(path);
	}
}

void Ozette::rename_file(std::string from, std::string to) {
	// Somebody has moved or renamed a file. If there is an editor
	// open for it, update our editor map.
	auto existing = _editors.find(Path::absolute(from));
	if (existing == _editors.end()) return;
	auto edrec = existing->second;
	_editors.erase(existing);
	_editors[Path::absolute(to)] = edrec;
}

void Ozette::close_file(std::string path) {
	auto iter = _editors.find(Path::absolute(path));
	if (iter != _editors.end()) {
		_shell.close_window(iter->second.window);
		_editors.erase(iter);
	}
}

void Ozette::find_in_file(std::string path, Editor::line_t index) {
	// For now we just jump to the specified line. Someday we will get a search
	// regex, so we can put the editor into find mode.
	auto edrec = open_editor(path);
	if (edrec.view) {
		Editor::location_t line(index, 0);
		Editor::Range sel(line, line);
		edrec.view->select(*edrec.window, sel);
	}
}

void Ozette::set_clipboard(std::string text) {
	_clipboard = text;
}

std::string Ozette::get_clipboard() {
	return _clipboard;
}

void Ozette::cache_read(std::string name, std::vector<std::string> &lines) {
	lines.clear();
	std::string str;
	std::ifstream file(_cache_dir + "/" + name);
	while (std::getline(file, str)) {
		lines.push_back(str);
	}
	file.close();
}

void Ozette::cache_write(std::string name, const std::vector<std::string> &l) {
	// if the ozette directory doesn't exist yet, create it
	struct stat st;
	if (stat(_cache_dir.c_str(), &st)) {
		int err = mkdir(_cache_dir.c_str(), S_IRWXU);
		// if we failed to create the directory, don't try wriing to it
		if (err) {
			return;
		}
	}
	std::ofstream file(_cache_dir + "/" + name, std::ios::trunc);
	for (auto &line: l) {
		file << line << std::endl;
	}
	file.close();
}

void Ozette::exec(std::string command) {
	std::vector<std::string> argv = {"-c", command};
	Console::View::exec(command, "sh", argv, _shell);
}

Ozette::editor Ozette::open_editor(std::string path) {
	// If we already have this file open, bring it forward.
	path = Path::absolute(path);
	auto existing = _editors.find(path);
	if (existing != _editors.end()) {
		_shell.make_active(existing->second.window);
		return existing->second;
	}
	// We don't have an editor for this file, so we should create one.
	editor edrec;
	edrec.view = new Editor::View(path);
	std::unique_ptr<UI::View> edptr(edrec.view);
	edrec.window = _shell.open_window(std::move(edptr));
	_editors[path] = edrec;
	return edrec;
}

void Ozette::search(Search::spec query) {
	// Make the search results prettier: if we're searching in the working
	// directory, use "." instead of the literal path.
	std::string canontree = Path::absolute(query.haystack);
	if (canontree == Path::absolute(_current_dir)) {
		query.haystack = ".";
	} else if (canontree == Path::absolute(_home_dir)) {
		query.haystack = "~";
	}
	Search::View::exec(query, _shell);
}

void Ozette::run() {
	if (_editors.empty()) show_browser();
	timeout(100);
	do {
		if (sig_io_flag.exchange(false)) {
			_shell.poll();
		}
		int ch = fix_control_quirks(getch());
		switch (ch) {
			case Control::UpArrow: show_browser(); break;
			case Control::NewFile: new_file(); break;
			case Control::Open: open_file(); break;
			case Control::Directory: change_directory(); break;
			case Control::Help: show_help(); break;
			case Control::Execute: execute(); break;
			case KEY_F(4): search(); break;
			case KEY_F(5): build(); break;
			default: _done |= !_shell.process(ch);
		}
		update_panels();
		doupdate();
	} while (!_done);
}

void Ozette::sig_io() {
	sig_io_flag.store(true);
}

void Ozette::show_browser() {
	Browser::View::open(_current_dir, _shell);
}

void Ozette::change_directory() {
	Dialog::Form dialog;
	dialog.fields = {
		{"Change Directory", Path::display(_current_dir), &Path::complete_dir}
	};
	dialog.commit = [this](UI::Frame &ctx, Dialog::Form::Result &res) {
		std::string path = res.selected_value;
		if (path.empty()) return;
		change_dir(path);
	};
	dialog.show(*_shell.active());
}

void Ozette::new_file() {
	// Is one of our editors the active window? If so, ask for the target path
	// for its document and use that as the default location for the new file.
	std::string path = _current_dir;
	for (auto wpair: _editors) {
		if (wpair.second.window == _shell.active()) {
			if (!wpair.first.empty()) {
				path = Path::absolute(wpair.first);
				size_t trunc = path.find_last_of('/');
				if (trunc == std::string::npos) {
					trunc = 0;
				}
				path.resize(trunc);
			}
			break;
		}
	}
	// Bring up the dialog allowing the user to enter a name for the file
	// and change its location if they wish.
	Dialog::Form dialog;
	dialog.fields = {
		{"New File", Path::display(path) + "/", &Path::complete_file}
	};
	dialog.commit = [this](UI::Frame &ctx, Dialog::Form::Result &result) {
		std::string path = Path::absolute(result.selected_value);
		if (path.empty()) {
			ctx.show_result("Cancelled");
			return;
		}
		editor edrec;
		edrec.view = new Editor::View(path);
		std::unique_ptr<UI::View> edptr(edrec.view);
		edrec.window = _shell.open_window(std::move(edptr));
		_editors[path] = edrec;
	};
	dialog.show(*_shell.active());
}

void Ozette::open_file() {
	Dialog::Form dialog;
	dialog.fields = {
		{"Open", "", &Path::complete_file}
	};
	dialog.commit = [this](UI::Frame &ctx, Dialog::Form::Result &res) {
		std::string path = res.selected_value;
		if (path.empty()) return;
		edit_file(path);
	};
	dialog.show(*_shell.active());
}

void Ozette::show_help() {
	Help::View::show(_shell);
}

void Ozette::execute() {
	Dialog::Form dialog;
	dialog.fields = {
		{"exec"}
	};
	dialog.commit = [this](UI::Frame &ctx, Dialog::Form::Result &res) {
		exec(res.selected_value);
	};
	dialog.show(*_shell.active());
}

void Ozette::build() {
	// Save all open editors. Execute the build command for this directory.
	for (auto &edit_pair: _editors) {
		edit_pair.second.window->process(Control::Save);
	}
	exec("make");
}

void Ozette::search() {
	show_browser();
	Search::spec job = {"", Path::display("."), "*"};
	Search::Dialog::show(*_shell.active(), job);
}

int Ozette::fix_control_quirks(int ch) {
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
	case 0x224:
	case 0x221:
	case 0x220: return Control::LeftArrow;
	case 0x22C:
	case 0x230:
	case 0x22F: return Control::RightArrow;
	case 0x20C:
	case 0x20D: return Control::DownArrow;
	case 0x235:
	case 0x236: return Control::UpArrow;

	default:
	return ch;
	}
}

