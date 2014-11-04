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
	bool line_visible(size_t index) const;
	unsigned line_columns(size_t index) const;
	int column_for_char(size_t index, size_t xoff) const;
	void reveal_cursor();
	void cursor_vert(int delta);
	void cursor_horz(int delta);
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
	size_t _cursy = 0;
	size_t _cursx = 0;
};

#endif // CONSOLE_H
