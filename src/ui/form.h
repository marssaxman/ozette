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
	Form(Field field): _fields(1, field) {}
	Form(std::initializer_list<Field> fields): _fields(fields) {}

	// When the user commits the form, it passes the result to a completion.
	struct Result {
		std::map<std::string, std::string> fields;
		size_t selection = 0;
		std::string selected_value;
	};
	typedef std::function<void(UI::Frame&, Result&)> action;
	void show(UI::Frame&, action);
private:
	std::vector<Field> _fields;
};
} // namespace UI

#endif //UI_FORM_H

