#ifndef EDITOR_EDITOR_H
#define EDITOR_EDITOR_H

#include "controller.h"
#include "document.h"
#include "update.h"
#include "cursor.h"

namespace Editor {
class Controller : public UI::Controller
{
public:
	Controller();
	Controller(std::string targetpath);
	virtual void activate(UI::Frame &ctx) override;
	virtual void paint(WINDOW *view, bool active) override;
	virtual bool process(UI::Frame &ctx, int ch) override;

private:
	void paint_line(WINDOW *view, row_t v, bool active);
	bool line_is_visible(line_t index) const;
	void reveal_cursor();
	void update_dimensions(WINDOW *view);
	void set_status(UI::Frame &ctx);

	// Control keys which drive higher-level functions.
	void ctl_cut(UI::Frame &ctx);
	void ctl_copy(UI::Frame &ctx);
	void ctl_paste(UI::Frame &ctx);
	void ctl_close(UI::Frame &ctx);
	void ctl_save(UI::Frame &ctx);
	void ctl_revert(UI::Frame &ctx);

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
	bool _local_changes = false;

	// Information about the editor window
	Update _update;
	Cursor _cursor;
	location_t _anchor;
	Range _selection;

	// Update status: must we perform a full repaint?
	bool _last_active = false;
	WINDOW *_last_dest = nullptr;
	// What are the width and height of the viewrect?
	size_t _width = 0;
	size_t _height = 0;
	size_t _halfheight = 0;
	size_t _maxscroll = 0;
	// What is the vertical position of the viewrect?
	size_t _scrollpos = 0;
};
} // namespace Editor

#endif // EDITOR_EDITOR_H
