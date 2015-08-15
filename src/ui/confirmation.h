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

#ifndef UI_CONFIRMATION_H
#define UI_CONFIRMATION_H

#include "ui/view.h"

// A dialog box asks the user a question, then waits for their answer.
// They may answer "yes" or "no", or they may cancel the operation that
// prompted the question.
namespace UI {
struct Confirmation {
	// A confirmation dialog asks the user a yes or no question.
	std::string text;
	// The user may answer yes or no.
	std::function<void(Frame&)> yes;
	std::function<void(Frame&)> no;
	// Once the confirmation is configured, show it within a window.
	void show(UI::Frame &ctx);
};
} // namespace UI

#endif UI_CONFIRMATION_H
