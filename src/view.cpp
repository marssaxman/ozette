#include "view.h"

void View::layout(WINDOW *window, int yoff, int xoff, int height, int width)
{
	_window = window;
	_yoff = yoff;
	_xoff = xoff;
	_height = height;
	_width = width;
	_blank.resize(width, ' ');
}

void View::move_cursor(int y, int x)
{
	move(std::min(y, _height) + _yoff, std::min(x, _width) + _xoff);
}

void View::fill(std::string text)
{
	size_t index = 0;
	size_t linestart = 0;
	size_t lineend = text.find_first_of('\n', linestart);
	while (lineend != std::string::npos) {
		size_t nextstart = lineend + 1;
		std::string line = text.substr(linestart, nextstart - linestart);
		write_line(index++, line);
		linestart = nextstart;
		lineend = text.find_first_of('\n', linestart);
	}
	std::string tail = text.substr(linestart, text.size() - linestart);
	if (index < (size_t)_height) {
		write_line(index, tail);
	}
	std::string blankline;
	while (++index < (size_t)_height) {
		write_line(index, blankline);
	}
}

void View::write_line(int index, std::string line)
{
	if (index < 0 || index >= _height) return;
	mvwaddnstr(_window, index + _yoff, _xoff, line.c_str(), _width);
	// clear the rest of the line
	int cury, curx;
	getyx(_window, cury, curx);
	curx -= _xoff;
	cury -= _yoff;
	if (cury == index) {
		while (curx++ < _width) {
			waddch(_window, ' ');
		}
	}
}

void View::clear_line(int index)
{
	if (index < 0 || index >= _height) return;
	mvwprintw(_window, index + _yoff, _xoff, _blank.c_str());
}
