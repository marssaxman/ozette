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

namespace UI {
class Form
{
public:
	struct Field {
		std::string name;
		std::string value;
		std::function<std::string(std::string)> completer;
	};
	Form(Field field): _fields(1, field) {}
	Form(std::initializer_list<Field> fields): _fields(fields) {}
	// Show the form, let the user edit, and return the values of all fields.
	typedef std::map<std::string, std::string> results_t;
	typedef std::function<void(UI::Frame&, results_t)> all_fields_t;
	void show(UI::Frame &ctx, all_fields_t action);
	// Show the form, let the user edit, and return the value of the field
	// which was selected when the user committed the form.
	typedef std::function<void(UI::Frame&, std::string)> selected_val_t;
	void show(UI::Frame &ctx, selected_val_t);
private:
	std::vector<Field> _fields;
};
} // namespace UI

#endif //UI_FORM_H

