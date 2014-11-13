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

#include "shell.h"
#include "control.h"
#include <algorithm>
#include <assert.h>
#include <cstring>

UI::Window::Window(App &app, std::unique_ptr<View> &&view):
	_app(app),
	_view(std::move(view)),
	_framewin(newwin(0, 0, 0, 0)),
	_framepanel(new_panel(_framewin))
{
	_view->activate(*this);
	paint();
}

UI::Window::~Window()
{
	del_panel(_framepanel);
	delwin(_framewin);
}

void UI::Window::layout(int xpos, int width)
{
	// Windows are vertical slices of screen space.
	// Given this column number and a width, compute the
	// window's nominal dimensions, then see if we need to adjust
	// the actual window to match.
	int screen_height, screen_width;
	getmaxyx(stdscr, screen_height, screen_width);
	// We will expand our edges by one pixel in each direction if
	// we have space for it so that we can draw a window frame.
	_lframe = xpos > 0;
	if (_lframe) {
		xpos--;
		width++;
	}
	_rframe = (xpos + width) < screen_width;
	if (_rframe) {
		width++;
	}
	_taskbar_height = HelpBar::Panel::kHeight;

	// What are the dimensions and location of our existing frame?
	// If the window has the wrong size, we must rebuild it, and
	// then we should layout our content window too. Otherwise, if
	// the window is in the wrong place, simply relocate it.
	int old_height, old_width;
	wmove(_framewin, 0, 0);
	getmaxyx(_framewin, old_height, old_width);
	int old_vert, old_horz;
	getbegyx(_framewin, old_vert, old_horz);
	if (old_height != screen_height || old_width != width) {
		WINDOW *replacement = newwin(screen_height, width, 0, xpos);
		replace_panel(_framepanel, replacement);
		delwin(_framewin);
		_framewin = replacement;
		_dirty_chrome = true;
	} else if (old_vert != 0 || old_horz != xpos) {
		move_panel(_framepanel, 0, xpos);
	}
	layout_contentwin();
	paint();
}

void UI::Window::set_focus()
{
	_view->activate(*this);
	_has_focus = true;
	_dirty_chrome = true;
	_dirty_content = true;
	if (_dialog) _dialog->activate(*this);
	paint();
}

void UI::Window::clear_focus()
{
	_view->deactivate(*this);
	_has_focus = false;
	_dirty_chrome = true;
	_dirty_content = true;
	if (_dialog) _dialog->deactivate(*this);
	paint();
}

void UI::Window::bring_forward(int focus_relative)
{
	top_panel(_framepanel);
	_view->bring_forward();
	if (_dialog) _dialog->bring_forward();
	bool swap_titlebar = focus_relative > 0;
	if (swap_titlebar != _swap_titlebar) {
		_swap_titlebar = swap_titlebar;
		_dirty_chrome = true;
		paint();
	}
}

bool UI::Window::process(int ch)
{
	if (ch != ERR) clear_result();
	if (_dialog) {
		// A dialog may invoke actions which may result in its own replacement.
		// We will temporarily move it into a local variable while it has
		// control, so we don't delete the object while its methods are on the
		// call stack.
		std::unique_ptr<View> temp(std::move(_dialog));
		if (temp->process(*this, ch)) {
			// If this dialog wants to stick around, we
			// expect that it hasn't done anything to change _dialog.
			assert(_dialog.get() == nullptr);
			_dialog = std::move(temp);
		} else {
			// If the dialog closed, that is sort of like
			// activating the window again, so tell the
			// controller to check up on whatever it was
			// doing.
			_view->activate(*this);
		}
		paint();
		return true;
	}
	bool out = _view->process(*this, ch);
	paint();
	return out;
}

void UI::Window::set_title(std::string text)
{
	_title = text;
	_dirty_chrome = true;
}

void UI::Window::set_status(std::string text)
{
	_status = text;
	_dirty_chrome = true;
}

void UI::Window::show_dialog(std::unique_ptr<View> &&host)
{
	clear_result();
	_dirty_chrome = true;
	_dialog = std::move(host);
	int vpos, hpos, height, width;
	calculate_content(vpos, hpos, height, width);
	_dialog->layout(vpos, hpos, height, width);
	_dirty_content = true;
}

void UI::Window::show_result(std::string message)
{
	clear_result();
	int numchars = message.size();
	int labelwidth = 2 + numchars + 2;
	int voff, hoff, height, width;
	calculate_content(voff, hoff, height, width);
	if (labelwidth > width) {
		labelwidth = width;
		numchars = labelwidth - 4;
	}
	voff += height - 1;
	hoff += (width - labelwidth) / 2;
	_resultwin = newwin(1, labelwidth, voff, hoff);
	_resultpanel = new_panel(_resultwin);
	wmove(_resultwin, 0, 0);
	wattron(_resultwin, A_REVERSE);
	waddstr(_resultwin, "[ ");
	waddnstr(_resultwin, message.c_str(), numchars);
	waddstr(_resultwin, " ]");
	curs_set(0);
}

void UI::Window::clear_result()
{
	if (!_resultwin) return;
	del_panel(_resultpanel);
	delwin(_resultwin);
	_resultpanel = nullptr;
	_resultwin = nullptr;
}

void UI::Window::calculate_content(int &vpos, int &hpos, int &height, int &width)
{
	// Compute the location and dimension of the content window,
	// relative to the location and dimension of the frame window.
	getmaxyx(_framewin, height, width);
	getbegyx(_framewin, vpos, hpos);

	// Adjust these dimensions inward to account for the space
	// used by the frame. Every window has a title bar.
	vpos++;
	height--;
	// The window may have a one-column left frame.
	if (_lframe) {
		hpos++;
		width--;
	}
	// The window may have a one-column right frame.
	if (_rframe) {
		width--;
	}
	// There may be a task bar, whose height may vary.
	height -= _taskbar_height;
}

void UI::Window::layout_contentwin()
{
	int new_vpos, new_hpos, new_height, new_width;
	calculate_content(new_vpos, new_hpos, new_height, new_width);
	_view->layout(new_vpos, new_hpos, new_height, new_width);
	_dirty_content = true;
	// If we have a dialog box open, tell it how to lay itself out.
	if (_dialog) {
		_dialog->layout(new_vpos, new_hpos, new_height, new_width);
	}
}

void UI::Window::layout_taskbar()
{
	unsigned new_height = 0;
	// The help panel occupies two lines
	new_height += HelpBar::Panel::kHeight;
	// Is that the height we planned for?
	if (new_height != _taskbar_height) {
		_taskbar_height = new_height;
		_dirty_chrome = true;
		layout_contentwin();
	}
}

void UI::Window::paint()
{
	if (_dirty_content) paint_content();
	if (_dirty_chrome) paint_chrome();
}

void UI::Window::paint_content()
{
	_view->paint(_has_focus);
	if (_dialog) _dialog->paint(_has_focus);
	_dirty_content = false;
}

void UI::Window::paint_chrome()
{
	int height, width;
	getmaxyx(_framewin, height, width);
	paint_titlebar(width);
	if (_lframe) paint_left_frame(height, width);
	if (_rframe) paint_right_frame(height, width);
	if (_taskbar_height) paint_taskbar(height, width);
	_dirty_chrome = false;
}

void UI::Window::paint_titlebar(int width)
{
	// Draw corners and a horizontal line across the top.
	mvwhline(_framewin, 0, 0, ACS_HLINE, width);
	if (_lframe) {
		mvwaddch(_framewin, 0, 0, ACS_ULCORNER);
	}
	if (_rframe) {
		mvwaddch(_framewin, 0, width-1, ACS_URCORNER);
	}
	paint_titlebar_left(width, _swap_titlebar? _status: _title);
	paint_titlebar_right(width, _swap_titlebar? _title: _status);
}

void UI::Window::paint_titlebar_left(int width, std::string text)
{
	if (text.empty()) return;
	int left = _lframe ? 2 : 1;
	int right = width - (_rframe ? 2 : 1);
	width = right - left;
	int titlechars = width - 2;
	if (_swap_titlebar) titlechars /= 2;
	wmove(_framewin, 0, left);
	if (_has_focus) wattron(_framewin, A_REVERSE);
	waddch(_framewin, ' ');
	waddnstr(_framewin, text.c_str(), titlechars);
	waddch(_framewin, ' ');
	if (_has_focus) wattroff(_framewin, A_REVERSE);
}

void UI::Window::paint_titlebar_right(int width, std::string text)
{
	if (text.empty()) return;
	int left = _lframe ? 2 : 1;
	int right = width - (_rframe ? 2 : 1);
	width = right - left;
	int titlechars = width - 2;
	if (!_swap_titlebar) titlechars /= 2;
	int chars = std::min((int)text.size(), titlechars);
	mvwaddch(_framewin, 0, right - chars - 2, ' ');
	waddnstr(_framewin, text.c_str(), chars);
	waddch(_framewin, ' ');
}

void UI::Window::paint_left_frame(int height, int width)
{
	mvwvline(_framewin, 1, 0, ACS_VLINE, height - 1);
}

void UI::Window::paint_right_frame(int height, int width)
{
	mvwvline(_framewin, 1, width-1, ACS_VLINE, height-1);
}

void UI::Window::paint_taskbar(int height, int width)
{
	HelpBar::Panel panel;
	if (_dialog) {
		_dialog->set_help(panel);
	} else {
		_view->set_help(panel);
	}

	int xpos = _lframe ? 1 : 0;
	width -= xpos;
	if (_rframe) width--;
	int ypos = height - _taskbar_height;
	// Clear out the space we're going to work in.
	for (int v = ypos; v < height; ++v) {
		mvwhline(_framewin, v, xpos, ' ', width);
	}

	// Render the help panel for this window.
	int labelwidth = width / HelpBar::Panel::kWidth;
	int textwidth = labelwidth - 4;
	unsigned v = 0;
	unsigned h = 0;

	int key_highlight = _has_focus ? A_REVERSE : 0;
	int cells = HelpBar::Panel::kWidth * HelpBar::Panel::kHeight;
	for (int i = 0; i < cells; ++i) {
		v = i / HelpBar::Panel::kWidth;
		h = i % HelpBar::Panel::kWidth;
		auto &label = panel.label[v][h];
		if (0 == label.mnemonic) continue;
		unsigned labelpos = h * labelwidth;
		wmove(_framewin, v+ypos, labelpos+xpos);
		wattron(_framewin, key_highlight);
		waddch(_framewin, label.is_ctrl? '^': ' ');
		waddch(_framewin, label.mnemonic);
		wattroff(_framewin, key_highlight);
		waddch(_framewin, ' ');
		waddnstr(_framewin, label.text.c_str(), textwidth);
		waddch(_framewin, ' ');
	}
}
