#include "dialog.h"

UI::Dialog::Dialog(std::unique_ptr<Controller> &&host):
	_win(newwin(0, 0, 0, 0)),
	_panel(new_panel(_win)),
	_host(std::move(host))
{
	Control::Panel ignored;
	_prompt = _host->open(ignored);
}

UI::Dialog::~Dialog()
{
	del_panel(_panel);
	delwin(_win);
}

void UI::Dialog::layout(int new_v, int new_h, int new_height, int new_width)
{
	// Given the specified content area, compute a reasonable
	// dimension for the dialog box. If the window needs to be resized,
	// blow it away and recreate it; otherwise, if it just needs to be
	// moved, move its panel.
	int old_height, old_width;
	getmaxyx(_win, old_height, old_width);
	int old_v, old_h;
	getbegyx(_win, old_v, old_h);
	// For the time being a dialog box is one line at the bottom of the
	// content area, and that's all there is to it. We will definitely
	// occupy the full width given to us, though.
	new_v = new_height - 1;
 	new_height = 1;
	if (new_height != old_height || new_width != old_width) {
		WINDOW *win = newwin(new_height, new_width, new_v, new_h);
		replace_panel(_panel, win);
		delwin(_win);
		_win = win;
	} else if (new_v != old_v || new_h != old_h) {
		move_panel(_panel, new_v, new_h);
	}
	paint();
}

void UI::Dialog::set_focus()
{
	_has_focus = true;
	paint();
}

void UI::Dialog::clear_focus()
{
	_has_focus = false;
	paint();
}

void UI::Dialog::bring_forward()
{
	top_panel(_panel);
}

bool UI::Dialog::process(int ch)
{
	// Every dialog can be dismissed via escape.
	if (ch == Control::Escape) return true;
	// Otherwise, we pass the keystroke along to the delegate.
	bool done = _host->process(ch);
	if (!done) paint();
	return done;
}

void UI::Dialog::paint()
{
	int height, width;
	getmaxyx(_win, height, width);
	(void)height; // unused
	wmove(_win, 0, 0);
	wattron(_win, A_REVERSE);
	waddnstr(_win, _prompt.c_str(), width);
	waddch(_win, ':');
	waddch(_win, ' ');
	std::string val = _host->value();
	int vpos, hpos;
	getyx(_win, vpos, hpos);
	(void)vpos; // unused
	waddnstr(_win, val.c_str(), width - hpos);
	getyx(_win, vpos, hpos);
	whline(_win, ' ', width - hpos);
	wattroff(_win, A_REVERSE);
	if (_has_focus) {
		wmove(_win, 0, hpos + _host->cursor());
		curs_set(1);
	} else {
		curs_set(0);
	}
}

