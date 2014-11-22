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

#ifndef UI_FRAME_H
#define UI_FRAME_H

#include <string>
#include <memory>
#include "controller.h"
#include "control.h"

namespace UI {
class View;
// A frame represents the portion of a window managed by the shell.
// This interface allows the window's controller to manipulate those
// elements of the window which refer to its content and not to its
// function within the shell.
class Frame {
public:
	virtual ~Frame() = default;
	virtual Controller &app() = 0;
	// The window content should be redrawn.
	virtual void repaint() = 0;
	// Change the title text, status text, or help text for the
	// window this controller is managing.
	virtual void set_title(std::string text) = 0;
	virtual void set_status(std::string text) = 0;
	// Open a dialog box and request input from the user.
	// The controller will be suspended while the dialog is open.
	virtual void show_dialog(std::unique_ptr<View> &&dialog) = 0;
	// Some process has completed. Show the result to the user as
	// a temporary floating result bar.
	virtual void show_result(std::string) = 0;
};
} // namespace UI

#endif // UI_FRAME_H
