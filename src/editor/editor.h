#ifndef EDITOR_H
#define EDITOR_H

#include "controller.h"
#include <vector>

class Editor : public Controller
{
public:
	Editor(std::string targetpath);
	virtual void paint(WINDOW *view, bool active) override;
	virtual bool process(WINDOW *view, int ch, App &app) override;
	virtual bool poll(WINDOW *view, App &app) override { return true; }
	virtual std::string title() const override { return _targetpath; }
protected:
	static const unsigned kTabWidth;
	class Update
	{
	public:
		void reset();
		void all();
		void line(size_t line);
		void range(size_t from, size_t to);
		bool has_dirty() const { return _dirty; }
		bool is_dirty(size_t line) const;
	private:
		bool _dirty = true;
		size_t _linestart = 0;
		size_t _lineend = 0;
	} _update;

	void paint_line(WINDOW *view, unsigned y);
	bool line_is_visible(size_t index) const;
	std::string get_line_text(size_t index) const;
	size_t get_line_size(size_t index) const;
	void reveal_cursor();
	void move_cursor_up(size_t lines);
	void move_cursor_down(size_t lines);
	void move_cursor_left();
	void move_cursor_right();
	void move_cursor_home(); // line-relative
	void move_cursor_end();	// line-relative
	size_t char_for_column(unsigned column, size_t line) const;
	unsigned column_for_char(size_t charoff, size_t line) const;
	unsigned char_width(char ch, size_t column) const;
	unsigned tab_width(size_t column) const;
	void update_dimensions(WINDOW *view);
private:
	std::string _targetpath;
	std::vector<std::string> _lines;
	bool _last_active = false;
	WINDOW *_last_dest = nullptr;
	// What are the width and height of the viewrect?
	size_t _width = 0;
	size_t _height = 0;
	size_t _halfheight = 0;
	size_t _maxscroll = 0;
	size_t _maxline = 0;	// ubound, not size
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

#endif // CONSOLE_H
