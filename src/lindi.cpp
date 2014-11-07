#include "lindi.h"
#include "browser.h"
#include "editor.h"
#include "control.h"

Lindi::Lindi():
	_shell(*this),
	_browser(new Browser)
{
        std::unique_ptr<UI::Controller> browser(_browser);
        _browserwindow = _shell.open_window(std::move(browser));
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

void Lindi::file_closed(std::string path)
{
	auto iter = _editors.find(path);
	if (iter != _editors.end()) _editors.erase(iter);
}

void Lindi::set_clipboard(std::string text)
{
	_clipboard = text;
}

std::string Lindi::get_clipboard()
{
	return _clipboard;
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
			case Control::Directory:
				// These commands are available anywhere,
				// but the browser is what handles them.
				activate_browser();
				// Now that the browser is active, let the
				// shell pass our message along normally.
			default: _done |= !_shell.process(ch);
		}
	} while (!_done);
}

void Lindi::new_file()
{
	std::unique_ptr<UI::Controller> ed(new Editor::Controller());
	_shell.open_window(std::move(ed));
}

void Lindi::activate_browser()
{
	_shell.make_active(_browserwindow);
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
