#include "ui.h"
#include <algorithm>

Window::Window(int height, int width):
	_window(newwin(height, width, 0, 0)),
	_panel(new_panel(_window))
{
}

Window::~Window()
{
	del_panel(_panel);
	delwin(_window);
}

void Window::move_to(int ypos, int xpos)
{
	move_panel(_panel, ypos, xpos);
}

Console::Console(int height, int width):
	Window(height, width)
{
	wprintw(_window, "Console\n");
}

bool Console::process(int ch)
{
	return false;
}

Browser::Browser(int height, int width):
	Window(height, width)
{
	wprintw(_window, "Browser\n");
}

bool Browser::process(int ch)
{
	return false;
}

UI::UI()
{
	// Set up ncurses.
	initscr();
	cbreak();
	noecho();
	nonl();
	intrflush(stdscr, FALSE);
	keypad(stdscr, true);
	// Find out how big the terminal is.
	getmaxyx(stdscr, _height, _width);
	// Build our two starting windows.
	// We reserve the top line for the title bar.
	const int ypos = 1;
	// The windows are never wider than 80 columns.
	int colwidth = std::min(_width, 80);
	int colheight = _height - ypos;
	_columns.emplace_back(new Console(colheight, colwidth));
	_columns.emplace_back(new Browser(colheight, colwidth));
	// Lay the windows out proportionally across the terminal.
	relayout();
}

UI::~UI()
{
	// Delete all of the windows.
	_columns.clear();
	// Clean up ncurses.
	endwin();
}

void UI::relayout()
{
	// The leftmost window owns column zero and covers no more than 80
	// characters' width.
	// Divide any remaining space among any remaining windows and stagger
	// each remaining window proportionally across the screen.
	_spacing = _width - 80;
	if (_columns.size() > 1) {
		_spacing /= (_columns.size() - 1);
	}
	for (unsigned i = 0; i < _columns.size(); ++i) {
		_columns[i]->move_to(1, _spacing * i);
	}
	drawtitlebar();
}

void UI::drawtitlebar()
{
	// Draw all the non-active titles using normal text.
	for (unsigned i = 0; i < _columns.size(); ++i) {
		if (i == _focus) continue;
		std::string title = preptitle(_columns[i]->title(), _spacing);
		mvprintw(0, i * _spacing, title.c_str());
	}
	// Draw the focus title bar in emphasis (reverse),
	// over the top of all other menu title bars.
	std::string focustitle = preptitle(_columns[_focus]->title(), 80);
	attron(A_REVERSE);
	mvprintw(0, _focus * _spacing, focustitle.c_str());
	attroff(A_REVERSE);
}

std::string UI::preptitle(std::string title, int barwidth)
{
	// Menu bar titles always begin and end with a space.
	// We will return a string whose length equals barwidth.
	title.resize(barwidth - 2, ' ');
	return " " + title + " ";
}

