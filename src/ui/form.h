//
// ozette
// Copyright (C) 2015 Mars J. Saxman
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

#ifndef UI_FORM_H
#define UI_FORM_H

#include <vector>
#include <memory>
#include <functional>
#include <initializer_list>
#include "ui/frame.h"
#include "ui/helpbar.h"

namespace UI {
class Form
{
public:
	// A form is a list of named input fields.
	struct Field {
		std::string name;
		std::string value;
		std::function<std::string(std::string)> completer;
	};
	std::vector<Field> fields;

	// The output values of the fields are collected into a result object.
	struct Result {
		std::map<std::string, std::string> fields;
		size_t selection = 0;
		std::string selected_value;
	};

	// If the user commits via Enter/Return, the primary action is invoked.
	typedef std::function<void(UI::Frame&, Result&)> action_t;
	action_t commit = nullptr;

	// An optional alternate commit action may be invoked with a control key.
	// A label will describe this action on the help bar.
	struct Alternate {
		int control_key = 0;
		UI::HelpBar::Label label;
		action_t action = nullptr;
	};
	Alternate secondary;

	Form() {}
	Form(std::initializer_list<Field> f, action_t action):
		fields(f), commit(action) {}
	void show(UI::Frame &ctx);
};
} // namespace UI

#endif //UI_FORM_H

