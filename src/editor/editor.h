// ozette
// Copyright (C) 2014-2025 Mars J. Saxman
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

#ifndef EDITOR_EDITOR_H
#define EDITOR_EDITOR_H

#include "app/syntax.h"
#include "editor/config.h"
#include "editor/document.h"
#include "editor/update.h"
#include "ui/view.h"

namespace Editor {
class View : public UI::View {
public:
	View();
	View(std::string targetpath);
	virtual void activate(UI::Frame &ctx) override;
	virtual void deactivate(UI::Frame &ctx) override;
	virtual bool process(UI::Frame &ctx, int ch) override;
	virtual void set_help(UI::HelpBar::Panel &panel) override;
	void select(UI::Frame &ctx, Range range);
	bool is_modified() const;
protected:
	virtual void paint_into(WINDOW *view, State state) override;
	virtual void clear_overlay() override;
private:
	void postprocess(UI::Frame &ctx);
	void paint_line(WINDOW *view, row_t v, State state);
	void reveal_cursor();
	void update_dimensions(WINDOW *view);
	void set_status(UI::Frame &ctx);

	// Control keys which drive higher-level functions.
	void ctl_cut(UI::Frame &ctx);
	void ctl_copy(UI::Frame &ctx);
	void ctl_paste(UI::Frame &ctx);
	void ctl_close(UI::Frame &ctx);
	void ctl_save(UI::Frame &ctx);
	void ctl_save_as(UI::Frame &ctx);
	void ctl_toline(UI::Frame &ctx);
	void ctl_find(UI::Frame &ctx);
	void ctl_replace(UI::Frame &ctx);
	void ctl_find_next(UI::Frame &ctx);
	void ctl_undo(UI::Frame &ctx);
	void ctl_redo(UI::Frame &ctx);
	void ctl_open_next(UI::Frame &ctx);

	// Data-entry keystrokes generally begin by deleting whatever was
	// previously selected and possibly replacing it with something else.
	void delete_selection();
	Range replace_selection(std::string);
	void key_insert(char ch);
	void key_tab(UI::Frame &ctx);
	void key_btab(UI::Frame &ctx);
	void key_enter(UI::Frame &ctx);
	void key_return(UI::Frame &ctx);
	void key_backspace(UI::Frame &ctx);
	void key_delete(UI::Frame &ctx);
	void key_escape(UI::Frame &ctx);

	// Place the cursor (and anchor) at a specific location in document space.
	void move_cursor(location_t loc);
	// Stretch the selection from the anchor to the new cursor point.
	void extend_selection(location_t loc);
	// Square the selection to the beginning and end of each line.
	void line_frame_selection();
	// On which screen column does this character location appear?
	column_t column(location_t);
	// Find some location relative to the cursor location.
	location_t arrow_up();
	location_t arrow_down();
	location_t arrow_left();
	location_t arrow_right();
	location_t page_up();
	location_t page_down();

	bool save(UI::Frame &ctx, std::string dest);
	bool find(UI::Frame &ctx, location_t anchor, std::string pattern);

	// Information about the file being edited
	std::string _targetpath;
	Document _doc;
	// Syntax for this document's file type
	const Syntax::Grammar &_syntax;

	// Information about the editor window
	Config _config;
	Update _update;
	location_t _cursor;
	location_t _anchor;
	Range _selection;
	std::string _find_text;
	std::string _replace_text;
	enum class FindNextAction {
		Nothing,
		Find,
		Replace
	} _find_next_action;

	// Update status: must we perform a full repaint?
	State _last_state = State::Inactive;
	WINDOW *_last_dest = nullptr;
	// What are the width and height of the viewrect?
	column_t _width = 0;
	row_t _height = 0;
	row_t _halfheight = 0;
	row_t _maxscroll = 0;
	// Where is the viewrect origin?
	position_t _scroll = {0,0};
};
} // namespace Editor

#endif // EDITOR_EDITOR_H
