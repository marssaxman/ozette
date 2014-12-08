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

#ifndef BROWSER_PICKER_H
#define BROWSER_PICKER_H

#include "dialog.h"

namespace Browser {
// Picker asks the user to enter a file path.
// It may be a directory path or a file name path, depending.
class Picker : public UI::Dialog::Input
{
	typedef UI::Dialog::Input inherited;
public:
	Picker(
		std::string prompt,
		std::vector<std::string> options,
		inherited::action_t commit);
	Picker(
		std::string prompt, std::string value, inherited::action_t commit);
	virtual void layout(int vpos, int hpos, int height, int width) override;
	virtual bool process(UI::Frame &ctx, int ch) override;
protected:
	virtual void paint_into(WINDOW *view, bool active) override;
	virtual void select_field();
	virtual bool field_selected() const override;
	std::vector<std::string> _options;
	bool _suggestion_selected = false;
	size_t _sugg_item = 0;
private:
	void select_suggestion(size_t i);
	void arrow_left(UI::Frame &ctx);
	void arrow_right(UI::Frame &ctx);
	void arrow_up();
	void arrow_down();
};
} // namespace Browser

#endif BROWSER_PICKER_H

