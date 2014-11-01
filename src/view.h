#ifndef VIEW_H
#define VIEW_H

#include <ncurses.h>
#include <string>
#include <vector>

class View
{
public:
	// What are the dimensions of this view?
	int width() const { return _width; }
	int height() const { return _height; }
	// The view should apply to a region of this window.
	void layout(WINDOW *dest, int yoff, int xoff, int height, int width);
	// The view should gain focus, changing appearance if necessary
	// and putting the selection or cursor into the appropriate location.
	void set_focus();
	// Starting at the top left corner of the view, write the contents
	// of this string, truncating lines at the width.
	void fill(std::string text);
	// Change the specified line to this string, truncating at the
	// width of the string.
	void write_line(int index, std::string text);
protected:
	void position_cursor();
	void blitline(int index, std::string line);
private:
	WINDOW *_window = nullptr;
	int _yoff = 0;
	int _xoff = 0;
	int _width = 0;
	int _height = 0;
	int _xcursor = 0;
	int _ycursor = 0;
};

#endif // VIEW_H
