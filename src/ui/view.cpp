#include "view.h"

UI::View::View():
	_window(newwin(0, 0, 0, 0)),
	_panel(new_panel(_window))
{
}

UI::View::~View()
{
	del_panel(_panel);
	delwin(_window);
}

void UI::View::layout(int vpos, int hpos, int height, int width)
{
	int old_height, old_width;
	getmaxyx(_window, old_height, old_width);
	int old_vpos, old_hpos;
	getbegyx(_window, old_vpos, old_hpos);
	if (old_height != height || old_width != width) {
		WINDOW *replacement = newwin(height, width, vpos, hpos);
		replace_panel(_panel, replacement);
		delwin(_window);
		_window = replacement;
	} else if (old_vpos != vpos || old_hpos != hpos) {
		move_panel(_panel, vpos, hpos);
	}
}

void UI::View::bring_forward()
{
	top_panel(_panel);
}

void UI::View::paint(bool active)
{
	wmove(_window, 0, 0);
	curs_set(0);
	paint_into(_window, active);
}
