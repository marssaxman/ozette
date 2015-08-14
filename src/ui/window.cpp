//
// ozette
// Copyright (C) 2014-2015 Mars J. Saxman
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

#include "ui/shell.h"
#include "app/control.h"
#include "ui/colors.h"
#include <algorithm>
#include <assert.h>
#include <cstring>

UI::Window::Window(Controller &app, std::unique_ptr<View> &&view):
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
	_view->deactivate(*this);
	del_panel(_framepanel);
	delwin(_framewin);
}

void UI::Window::layout(int xpos, int width)
{
	clear_result();
	// Windows are vertical slices of screen space.
	// Given this column number and a width, compute the
	// window's nominal dimensions, then see if we need to adjust
	// the actual window to match.
	int screen_height, screen_width;
	wmove(stdscr, 0, 0);
	getmaxyx(stdscr, screen_height, screen_width);
	// We will expand our edges by one pixel in each direction if
	// we have space for it so that we can draw a window frame.
	bool new_lframe = xpos > 0;
	if (new_lframe) {
		xpos--;
		width++;
	}
	_lframe = new_lframe;
	bool new_rframe = (xpos + width) < screen_width;
	if (new_rframe) {
		width++;
	}
	_rframe = new_rframe;
	_helpbar_height = HelpBar::Panel::kHeight;

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
	} else if (old_vert != 0 || old_horz != xpos) {
		move_panel(_framepanel, 0, xpos);
	}
	layout_contentwin();
	_dirty_chrome = true;
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
	clear_result();
	_has_focus = false;
	_dirty_chrome = true;
	_dirty_content = true;
	if (_dialog) _dialog->deactivate(*this);
	paint();
}

void UI::Window::bring_forward(FocusRelative rel)
{
	top_panel(_framepanel);
	_view->bring_forward();
	if (_dialog) _dialog->bring_forward();
	bool swap_titlebar = rel == FocusRelative::Right;
	if (swap_titlebar != _swap_titlebar) {
		_swap_titlebar = swap_titlebar;
		_dirty_chrome = true;
		paint();
	}
}

bool UI::Window::process(int ch)
{
	clear_result();
	// A signal to close the window implicitly cancels any open dialog.
	if (ch == Control::Close && _dialog) {
		close_dialog();
	}
	// If there's a dialog open, it gets to handle this event. Otherwise, the
	// window's own view will process it.
	bool more = true;
	if (_dialog) {
		process_dialog(ch);
	} else {
		more = _view->process(*this, ch);
	}
	paint();
	return more;
}

bool UI::Window::poll()
{
	bool more = true;
	if (_dialog) {
		std::unique_ptr<View> temp(std::move(_dialog));
		if (temp->poll(*this)) {
			assert(_dialog.get() == nullptr);
			_dialog = std::move(temp);
		} else {
			_view->activate(*this);
		}
	} else {
		more = _view->poll(*this);
	}
	paint();
	return more;
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
	_result_text = message;
	_dirty_content = true;
}

void UI::Window::clear_result()
{
	if (_result_text.empty()) return;
	_result_text.clear();
	_view->clear_overlay();
	_dirty_content = true;
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
	height -= _helpbar_height;
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

void UI::Window::layout_helpbar()
{
	unsigned new_height = 0;
	// The help panel occupies two lines
	new_height += HelpBar::Panel::kHeight;
	// Is that the height we planned for?
	if (new_height != _helpbar_height) {
		_helpbar_height = new_height;
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
	// We must draw the content from back to front: view, result, dialog.
	View::State content_state = View::State::Focused;
	if (_dialog) {
		content_state = View::State::Active;
	}
	if (!_has_focus) {
		content_state = View::State::Inactive;
	}
	_view->paint(content_state);

	View::State overlay_state = View::State::Focused;
	if (!_has_focus) {
		overlay_state = View::State::Inactive;
	}
	if (!_result_text.empty()) {
		_view->overlay_result(_result_text, overlay_state);
	}
	if (_dialog) {
		_dialog->paint(overlay_state);
	}
	_dirty_content = false;
}

void UI::Window::paint_chrome()
{
	wattrset(_framewin, Colors::chrome(_has_focus && !_dialog));
	int height, width;
	getmaxyx(_framewin, height, width);
	paint_titlebar(width);
	if (_lframe) {
		mvwvline(_framewin, 1, 0, ACS_VLINE, height - 1);
	}
	if (_rframe) {
		mvwvline(_framewin, 1, width-1, ACS_VLINE, height-1);
	}
	if (_helpbar_height) {
		// The task bar is still active when a dialog is open, because it shows
		// context-specific information.
		wattrset(_framewin, Colors::chrome(_has_focus));
		paint_helpbar(height, width);
	}
	_dirty_chrome = false;
	wstandend(_framewin);
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
	std::string left_text = _swap_titlebar? _status: _title;
	std::string right_text = _swap_titlebar? _title: _status;

	int left = _lframe ? 2 : 1;
	int right = width - (_rframe ? 2 : 1);
	width = right - left;
	// If we don't have enough space to display the title and the status,
	// truncate the longer of the two until they both fit, starting at the
	// beginning of the string and preserving the end. Add two to each string
	// length to account for the padding on either side.
	int titlechars = width - 2;
	int desired_width = left_text.size() + 2 + right_text.size();
	if (desired_width > titlechars) {
		size_t surplus = static_cast<size_t>(desired_width - titlechars);
		if (left_text.size() > right_text.size()) {
			left_text = ltrunc(left_text, surplus);
		} else {
			right_text = ltrunc(right_text, surplus);
		}
	}
	if (!left_text.empty()) {
		wmove(_framewin, 0, left);
		bool highlight = _has_focus && !_dialog.get();
		if (highlight) wattron(_framewin, A_REVERSE);
		waddch(_framewin, ' ');
		waddnstr(_framewin, left_text.c_str(), titlechars);
		waddch(_framewin, ' ');
		if (highlight) wattroff(_framewin, A_REVERSE);
	}
	if (!right_text.empty()) {
		int chars = std::min((int)right_text.size(), titlechars);
		mvwaddch(_framewin, 0, right - chars - 2, ' ');
		waddnstr(_framewin, right_text.c_str(), chars);
		waddch(_framewin, ' ');
	}
}

void UI::Window::paint_helpbar(int height, int width)
{
	HelpBar::Panel panel;
	if (_dialog) {
		_dialog->set_help(panel);
		// The window handles dialog cancel via escape, so we'll add that
		// help tag to the panel ourselves.
		panel.escape();
	} else {
		// Windows get "close" by default, but they can change it if need be.
		panel.close();
		_view->set_help(panel);
		// The help command, however, is mandatory.
		panel.help();
	}

	int xpos = _lframe ? 1 : 0;
	width -= xpos;
	if (_rframe) width--;
	int ypos = height - _helpbar_height;
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
		if (0 == label.mnemonic[0]) continue;
		unsigned labelpos = h * labelwidth;
		wmove(_framewin, v+ypos, labelpos+xpos);
		wattron(_framewin, key_highlight);
		waddch(_framewin, label.mnemonic[0]);
		waddch(_framewin, label.mnemonic[1]);
		wattroff(_framewin, key_highlight);
		waddch(_framewin, ' ');
		waddnstr(_framewin, label.text.c_str(), textwidth);
		waddch(_framewin, ' ');
	}
}

std::string UI::Window::ltrunc(const std::string &text, size_t surplus)
{
	return (text.size() > surplus)? text.substr(surplus): "";
}

void UI::Window::process_dialog(int ch)
{
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
		_dirty_content = true;
	}
}

void UI::Window::close_dialog()
{
	_dialog.reset(nullptr);
	_view->activate(*this);
	_dirty_content = true;
}
