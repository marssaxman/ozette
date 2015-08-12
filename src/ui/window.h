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

#ifndef UI_WINDOW_H
#define UI_WINDOW_H

#include <ncurses.h>
#include <panel.h>
#include <memory>
#include <vector>
#include "ui/view.h"
#include "app/controller.h"
#include "ui/helpbar.h"

namespace UI {
class Window : public Frame
{
public:
	Window(Controller &app, std::unique_ptr<View> &&view);
	~Window();
	void layout(int xpos, int width);
	void set_focus();
	void clear_focus();
	enum class FocusRelative {
		Left,
		Equal,
		Right
	};
	void bring_forward(FocusRelative rel);
	bool process(int ch);
	bool poll();
	virtual void show_dialog(std::unique_ptr<View> &&dialog) override;
	View::Priority priority() const { return _view->priority(); }
protected:
	virtual void repaint() override { _dirty_content = true; }
	virtual Controller &app() override { return _app; }
	virtual void set_title(std::string text) override;
	virtual void set_status(std::string text) override;
	virtual void show_result(std::string message) override;
	void clear_result();
	void calculate_content(int &vpos, int &hpos, int &height, int &width);
	void layout_contentwin();
	void layout_helpbar();
	void paint();
	void paint_content();
	void paint_chrome();
	void paint_titlebar(int width);
	void paint_helpbar(int height, int width);
private:
	void process_dialog(int ch);
	void close_dialog();
	static std::string ltrunc(const std::string &text, size_t surplus);
	Controller &_app;
	std::unique_ptr<View> _view;
	// The frame represents the outer dimensions of the window and
	// contains all the UI chrome.
	WINDOW *_framewin = nullptr;
	PANEL *_framepanel = nullptr;
	// There may be a dialog box overlaid on the content window, if the
	// user is currently engaged in some process which requires input.
	std::unique_ptr<View> _dialog;
	// There may be an ephemeral result blurb overlaid on the content window.
	std::string _result_text;
	// Are we the active window? This changes the way we draw our chrome.
	bool _has_focus = false;
	// What are the dimensional attributes we worked out during layout?
	bool _lframe = false;
	bool _rframe = false;
	unsigned _helpbar_height = 0;
	// Have we experienced changes which require repainting but which we
	// have not yet had a chance to implement?
	bool _dirty_content = true;
	bool _dirty_chrome = true;
	// Data to display in our window chrome: this is for the use of our
	// controller object, which can update these fields as it pleases.
	std::string _title;
	std::string _status;
	bool _swap_titlebar = false;
};
} // namespace UI

#endif // UI_WINDOW_H
