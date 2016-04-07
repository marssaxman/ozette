// ozette
// Copyright (C) 2014-2016 Mars J. Saxman
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

#include "app/ozette.h"
#include "app/path.h"
#include "browser/browser.h"
#include "editor/editor.h"
#include "console/console.h"
#include "search/search.h"
#include "search/dialog.h"
#include "app/control.h"
#include "app/help.h"
#include <unistd.h>
#include <fstream>
#include <sys/stat.h>
#include <assert.h>

Ozette::Ozette():
	_shell(*this),
	_home_dir(getenv("HOME")),
	_config_dir(_home_dir + "/.ozette"),
	_config(_config_dir, ".") {
	char *cwd = getcwd(NULL, 0);
	if (cwd) {
		_current_dir = cwd;
		free(cwd);
	} else {
		_current_dir = _home_dir;
	}
	_config.change_directory(_current_dir);
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
	_config.change_directory(path);
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
	std::ifstream file(_config_dir + "/" + name);
	while (std::getline(file, str)) {
		lines.push_back(str);
	}
	file.close();
}

void Ozette::cache_write(std::string name, const std::vector<std::string> &l) {
	// if the ozette directory doesn't exist yet, create it
	struct stat st;
	if (stat(_config_dir.c_str(), &st)) {
		int err = mkdir(_config_dir.c_str(), S_IRWXU);
		assert(0 == err);
	}
	std::ofstream file(_config_dir + "/" + name, std::ios::trunc);
	for (auto &line: l) {
		file << line << std::endl;
	}
	file.close();
}

Config &Ozette::config() {
	return _config;
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
	edrec.view = new Editor::View(path, _config);
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
			case KEY_F(4): search(); break;
			case KEY_F(5): build(); break;
			default: _done |= !_shell.process(ch);
		}
	} while (!_done);
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
	editor edrec;
	edrec.view = new Editor::View(_config);
	std::unique_ptr<UI::View> edptr(edrec.view);
	edrec.window = _shell.open_window(std::move(edptr));
	_editors[Path::absolute("")] = edrec;
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
	static const std::string help_key = " Help ";
	static const std::string abs_help = Path::absolute(help_key);
	auto existing = _editors.find(abs_help);
	if (existing != _editors.end()) {
		_shell.make_active(existing->second.window);
		return;
	}
	std::string helptext((const char*)HELP, HELP_len);
	helptext += "\n";
	Editor::Document doc(_config);
	doc.View(helptext);
	editor edrec;
	edrec.view = new Editor::View(help_key, std::move(doc), _config);
	std::unique_ptr<UI::View> edptr(edrec.view);
	edrec.window = _shell.open_window(std::move(edptr));
	_editors[abs_help] = edrec;
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
	std::string command = _config.get("build-command", "make");
	exec(command);
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
	case 0x20C: return Control::DownArrow;
	case 0x235: return Control::UpArrow;

	default:
	return ch;
	}
}

