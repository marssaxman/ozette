#ifndef EDITOR_EDITOR_H
#define EDITOR_EDITOR_H

#include "controller.h"
#include "document.h"
#include "update.h"

namespace Editor {
class Controller : public ::Controller
{
public:
	Controller(std::string targetpath);
	virtual void paint(WINDOW *view, bool active) override;
	virtual bool process(WINDOW *view, int ch, App &app) override;
	virtual bool poll(WINDOW *view, App &app) override { return true; }
	virtual std::string title() const override { return _targetpath; }
protected:

	void paint_line(WINDOW *view, unsigned y);
	bool line_is_visible(size_t index) const;
	void reveal_cursor();
	void move_cursor_up(size_t lines);
	void move_cursor_down(size_t lines);
	void move_cursor_left();
	void move_cursor_right();
	void move_cursor_home(); // line-relative
	void move_cursor_end();	// line-relative
	void update_dimensions(WINDOW *view);
private:
	std::string _targetpath;
	Document _doc;
	Update _update;
	bool _last_active = false;
	WINDOW *_last_dest = nullptr;
	// What are the width and height of the viewrect?
	size_t _width = 0;
	size_t _height = 0;
	size_t _halfheight = 0;
	size_t _maxscroll = 0;
	// What is the vertical position of the viewrect?
	size_t _scrollpos = 0;
	// Where is the cursor located within the document?
	// We need to record *both* the screen column and the
	// character offset that column represents, because
	// columns don't map evenly to characters - tabs may
	// expand to a variable number of columns.
	size_t _curs_line = 0;
	size_t _curs_char = 0; // relative to the line text
	unsigned _curs_col = 0; // position on screen
};
} // namespace Editor

#endif // EDITOR_EDITOR_H
