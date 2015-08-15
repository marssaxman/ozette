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

#ifndef EDITOR_FINDER_H
#define EDITOR_FINDER_H

#include "ui/view.h"
#include "editor/editor.h"
#include "ui/input.h"

namespace Editor {
class Finder : public UI::View
{
	typedef UI::View inherited;
public:
	static Finder* find(Editor::View&, Document&, Range selection);
	static Finder* find_next(UI::Frame&, Editor::View&, Document&, Range sel);
	virtual void layout(int vpos, int hpos, int height, int width) override;
	virtual bool process(UI::Frame &ctx, int ch) override;
	virtual void set_help(UI::HelpBar::Panel &panel) override;
protected:
	// Open a finder and wait for input.
	Finder(Editor::View&, Document&, Range selection);
	// Open a finder and immediately search for the selected text.
	Finder(UI::Frame&, Editor::View&, Document&, Range selection);
	virtual void paint_into(WINDOW *view, State state) override;
	void input_changed(UI::Frame &ctx);
	void run_find(UI::Frame &ctx);
	void find_next(UI::Frame &ctx);
private:
	Editor::View &_editor;
	Editor::Document &_document;
	Editor::Range _original_selection;
	Editor::Range _anchor_selection;
	std::unique_ptr<UI::Input> _input;
	std::vector<Range> _matches;
	size_t _found_item;
};
} // namespace Editor

#endif //EDITOR_FINDER_H

