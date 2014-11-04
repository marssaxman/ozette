#include "lindi.h"
#include "browser.h"
#include "editor.h"

Lindi::Lindi():
	_ui(*this),
	_browser(new Browser)
{
        std::unique_ptr<Controller> browser(_browser);
        _ui.open_window(std::move(browser));
}

void Lindi::edit_file(std::string path)
{
	// If we already have this file open, bring it forward.
	auto existing = _editors.find(path);
	if (existing != _editors.end()) {
		_ui.make_active(existing->second);
		return;
	}
	// We don't have an editor for this file, so we should create one.
	std::unique_ptr<Controller> ed(new Editor(path));
	Window *win = _ui.open_window(std::move(ed));
	_editors[path] = win;
}

void Lindi::select_project(std::string path)
{
	_browser->open_project(path);
}

void Lindi::run()
{
	if (_editors.empty()) {
		_browser->show_projects();
	}
	timeout(20);
	do {
		update_panels();
		doupdate();
	} while (!_done && _ui.process(getch(), *this));
}

void Lindi::window_closed(std::unique_ptr<Window> &&win)
{
	Window *wptr = win.get();
	for (auto iter = _editors.begin(); iter != _editors.end(); ++iter) {
		if (iter->second == wptr) {
			_editors.erase(iter);
			break;
		}
	}
}
