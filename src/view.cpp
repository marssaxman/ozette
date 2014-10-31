#include "view.h"

void View::layout(WINDOW *window, int yoff, int xoff, int height, int width)
{
	_window = window;
	_yoff = yoff;
	_xoff = xoff;
	_height = height;
	_width = width;
	position_cursor();
}

void View::set_focus()
{
	position_cursor();
}

void View::position_cursor()
{
	move(_ycursor + _yoff, _xcursor + _xoff);
}

void View::fill(std::string text)
{
	mvwprintw(_window, _yoff, _xoff, "y: %d, x: %d, width: %d, height: %d", _yoff, _xoff, _width, _height);
	mvwprintw(_window, _yoff+1, _xoff, "hello!");
	return;
	size_t index = 0;
	size_t linestart = 0;
	size_t lineend = text.find_first_of('\n', linestart);
	while (lineend != std::string::npos) {
		size_t nextstart = lineend + 1;
		std::string line = text.substr(linestart, nextstart - linestart);
		blitline(index, line);
		index++;
		linestart = nextstart;
	}
	std::string tail = text.substr(linestart, text.size() - linestart);
	if (index < (size_t)_height) {
		blitline(index, tail);
	}
	_xcursor = index;
	_ycursor = 0;
	std::string blankline;
	while (++index < (size_t)_height) {
		blitline(index, blankline);
	}
	position_cursor();
}

void View::blitline(int index, std::string line)
{
	if (index < 0 || index >= _height) return;
	mvwprintw(_window, index + _yoff, _xoff, "%d (%d)", index, line.size());
}
