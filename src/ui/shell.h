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

#ifndef UI_SHELL_H
#define UI_SHELL_H

#include "window.h"
#include <vector>
#include <queue>

namespace UI {
class Shell
{
public:
	Shell(Controller &app);
	~Shell();
	bool process(int ch);
	Window *open_window(std::unique_ptr<View> &&view);
	void close_window(Window *window);
	void make_active(Window *window);
	Window *active() const { return _tabs[_focus].get(); }
protected:
	void reap();
	// change the focus to a specific window
	void set_focus(size_t index);
	// position all the windows after create/remove/resize
	void layout();
	// send this char to the focus window
	void send_to_focus(int ch);
	// close the window with this index
	void close_window(size_t index);
private:
	Controller &_app;
	int _width = 0;
	int _height = 0;
	std::vector<std::unique_ptr<Window>> _tabs;
	int _spacing = 0;
	int _columnWidth = 0;
	size_t _focus = 0;
	std::queue<std::unique_ptr<Window>> _doomed;
};
} // namespace UI

#endif // UI_SHELL_H
