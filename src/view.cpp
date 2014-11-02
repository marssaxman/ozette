#include "view.h"
#include <assert.h>

void View::layout(WINDOW *window)
{
	_window = window;
	getmaxyx(window, _height, _width);
	_blank.resize(_width, ' ');
}

void View::move_cursor(int y, int x)
{
	move(std::min(y, _height), std::min(x, _width));
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
	mvwaddnstr(_window, index, 0, line.c_str(), _width);
	// clear the rest of the line
	int cury, curx;
	getyx(_window, cury, curx);
	if (cury == index) {
		while (curx++ < _width) {
			waddch(_window, ' ');
		}
	}
}

void View::clear_line(int index)
{
	if (index < 0 || index >= _height) return;
	mvwprintw(_window, index, 0, _blank.c_str());
}
