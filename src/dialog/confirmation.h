// ozette
// Copyright (C) 2014-2016 Mars J. Saxman
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

#ifndef DIALOG_CONFIRMATION_H
#define DIALOG_CONFIRMATION_H

#include <functional>
#include "ui/view.h"

// A confirmation box asks the user a question, then waits for their answer.
// They may answer "yes" or "no", or they may cancel the operation that
// prompted the question.
namespace Dialog {
struct Confirmation {
	// A confirmation dialog asks the user a yes or no question.
	std::string text;
	// The question may include some lines of explanatory text.
	std::vector<std::string> supplement;
	// The user may answer yes or no.
	std::function<void(UI::Frame&)> yes;
	std::function<void(UI::Frame&)> no;
	// Once the confirmation is configured, show it within a window.
	void show(UI::Frame &ctx);
};
} // namespace Dialog

#endif //DIALOG_CONFIRMATION_H

