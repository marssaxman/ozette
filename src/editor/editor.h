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

#ifndef EDITOR_EDITOR_H
#define EDITOR_EDITOR_H

#include "view.h"
#include "document.h"
#include "update.h"
#include "cursor.h"
#include "config.h"

namespace Editor {
class View : public UI::View
{
public:
	View(const ::Config::All &config);
	View(std::string targetpath, const ::Config::All &config);
	View(std::string title, Document &&doc);
	virtual void activate(UI::Frame &ctx) override;
	virtual void deactivate(UI::Frame &ctx) override;
	virtual bool process(UI::Frame &ctx, int ch) override;
	virtual void set_help(UI::HelpBar::Panel &panel) override;
protected:
	virtual void paint_into(WINDOW *view, State state) override;
	virtual void clear_overlay() override;
private:
	void postprocess(UI::Frame &ctx);
	void paint_line(WINDOW *view, row_t v, bool active);
	void reveal_cursor();
	void update_dimensions(WINDOW *view);
	void set_status(UI::Frame &ctx);

	// Control keys which drive higher-level functions.
	void ctl_cut(UI::Frame &ctx);
	void ctl_copy(UI::Frame &ctx);
	void ctl_paste(UI::Frame &ctx);
	void ctl_close(UI::Frame &ctx);
	void ctl_save(UI::Frame &ctx);
	void ctl_toline(UI::Frame &ctx);
	void ctl_find(UI::Frame &ctx);
	void ctl_find_next(UI::Frame &ctx);
	void ctl_undo(UI::Frame &ctx);
	void ctl_redo(UI::Frame &ctx);
	void ctl_open_next(UI::Frame &ctx);

	// Navigation keystrokes move the cursor around the document.
	void key_up(bool extend);
	void key_down(bool extend);
	void key_left(bool extend);
	void key_right(bool extend);
	void key_page_up();
	void key_page_down();
	void key_home();
	void key_end();

	// Data-entry keystrokes generally begin by deleting whatever was
	// previously selected and possibly replacing it with something else.
	void delete_selection();
	void key_insert(char ch);
	void key_tab(UI::Frame &ctx);
	void key_btab(UI::Frame &ctx);
	void key_enter(UI::Frame &ctx);
	void key_return(UI::Frame &ctx);
	void key_backspace(UI::Frame &ctx);
	void key_delete(UI::Frame &ctx);

	// If there was a selection, pressing a key usually does something
	// to it, so we need to release the selection and start over with a
	// simple cursor. The only exception is when we've extended the
	// selection using shift-arrow-key movement.
	void drop_selection();
	void adjust_selection(bool extend);

	// File management basics.
	void save(UI::Frame &ctx, std::string dest);

	// Information about the file being edited
	std::string _targetpath;
	Document _doc;

	// Information about the editor window
	Update _update;
	Cursor _cursor;
	location_t _anchor;
	Range _selection;
	std::string _find_text;

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
