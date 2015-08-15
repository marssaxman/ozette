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

#include "editor/finder.h"
#include "ui/colors.h"

Editor::Finder::Finder(Editor::View &editor, Document &doc, Range selection):
	inherited(),
	_editor(editor),
	_document(doc),
	_original_selection(selection),
	_anchor_selection(selection)
{
	auto updater = [this](UI::Frame &ctx){ input_changed(ctx); };
	_input.reset(new UI::Input("", nullptr, updater));
}

void Editor::Finder::layout(int vpos, int hpos, int height, int width)
{
	inherited::layout(vpos + height - 1, hpos, 1, width);
}

bool Editor::Finder::process(UI::Frame &ctx, int ch)
{
	switch (ch) {
		case Control::Escape: {
			_editor.select(ctx, _original_selection);
			return false;
		}
		case KEY_F(9): run_find(ctx); break;
		case Control::Enter:
		case Control::Return: return false;
		case Control::FindNext: find_next(ctx); break;
		default:
			_input->process(ctx, ch);
	}
	return true;
}

void Editor::Finder::set_help(UI::HelpBar::Panel &panel)
{
	_input->set_help(panel);
	panel.find_next();
}

void Editor::Finder::paint_into(WINDOW *view, State state)
{

	int height, width;
	getmaxyx(view, height, width);
	(void)height;
	wattrset(view, UI::Colors::dialog(state == State::Focused));

	// Draw the caption on the left.
	std::string caption = "Find: ";
	mvwaddnstr(view, 0, 0, caption.c_str(), width);
	int fieldpos = std::min((int)caption.size(), width);
	// If there's still room, draw the location string on the right.
	std::string location;
	if (!_input->value().empty()) {
		location = " (";
		if (_matches.empty()) {
			location += "None found";
		} else {
			location += std::to_string(1 + _found_item);
			location += " of ";
			location += std::to_string(_matches.size());
		}
		location += ")";
	}
	int locpos = std::max(fieldpos, width - (int)location.size());
	mvwaddnstr(view, 0, locpos, location.c_str(), width - locpos);
	// Draw the input field in between caption and location.
	_input->paint(view, 0, fieldpos, locpos - fieldpos, state);
}

void Editor::Finder::input_changed(UI::Frame &ctx)
{
	run_find(ctx);
}

void Editor::Finder::run_find(UI::Frame &ctx)
{
	// Find all locations in the document which match the current pattern.
	// Identify the location immediately after the current anchor location.
	// Tell the editor to select that location. Update the "X of Y" status.
	std::string pattern = _input->value();
	if (pattern.empty()) {
		_matches.clear();
		_found_item = 0;
		_editor.select(ctx, _anchor_selection);
		return;
	}
	location_t anchorpoint = _anchor_selection.begin();
	_matches = _document.find(_input->value());
	_found_item = 0;
	for (auto &match: _matches) {
		if (match.begin() > anchorpoint) break;
		_found_item++;
	}
	find_next(ctx);
}

void Editor::Finder::find_next(UI::Frame &ctx)
{
	if (_matches.empty()) return;
	_found_item++;
	if (_found_item >= _matches.size()) {
		_found_item = 0;
	}
	_editor.select(ctx, _matches[_found_item]);
}

