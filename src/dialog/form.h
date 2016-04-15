// ozette
// Copyright (C) 2015-2016 Mars J. Saxman
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

#ifndef DIALOG_FORM_H
#define DIALOG_FORM_H

#include <vector>
#include <memory>
#include <functional>
#include <initializer_list>
#include "ui/frame.h"
#include "ui/helpbar.h"

namespace Dialog {
struct Form {
	// A form is a list of named input fields.
	struct Field {
		std::string name;
		std::string value;
		std::function<std::string(std::string)> completer;
		std::function<void(std::string)> updater;
	};
	std::vector<Field> fields;

	// The output values of the fields are collected into a result object.
	struct Result {
		std::map<std::string, std::string> fields;
		size_t selection = 0;
		std::string selected_value;
	};

	// Return/enter invokes the commit action; escape invokes the cancel
	// action (which is generally omitted as the default is to do nothing).
	typedef std::function<void(UI::Frame&, Result&)> action_t;
	action_t commit = nullptr;
	typedef std::function<void(UI::Frame&)> restore_t;
	restore_t cancel = nullptr;

	// Once the form is configured, show it within a host window.
	void show(UI::Frame &ctx);
};
} // namespace Dialog

#endif //DIALOG_FORM_H

