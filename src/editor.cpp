#include "editor.h"
#include <fstream>
#include "view.h"

Editor::Editor(std::string targetpath):
	_targetpath(targetpath)
{
	std::string str;
	std::ifstream file(targetpath);
	while (std::getline(file, str)) {
		_lines.push_back(str);
	}
}

void Editor::paint(WINDOW *dest)
{
	View view(dest);
	size_t effscroll = std::min(maxscroll(dest), _scrollpos);
	for (int i = 0; i < view.height(); ++i) {
		size_t line = i + effscroll;
		if (line < _lines.size()) {
			view.write_line(i, _lines[line]);
		} else {
			view.clear_line(i);
		}
	}
}

bool Editor::process(WINDOW *dest, int ch)
{
	switch (ch) {
		case 258: arrow_down(dest); break;
		case 259: arrow_up(dest); break;
		case 338: page_down(dest); break;
		case 339: page_up(dest); break;
		default: break;
	}
	return true;
}

void Editor::arrow_down(WINDOW *dest)
{
	if (_scrollpos < maxscroll(dest)) {
		_scrollpos++;
		paint(dest);
	}
}

void Editor::arrow_up(WINDOW *dest)
{
	size_t minscroll = 0;
	if (_scrollpos > minscroll) {
		_scrollpos--;
		paint(dest);
	}
}

void Editor::page_down(WINDOW *dest)
{
	View view(dest);
	size_t step = view.height() / 2;
	_scrollpos = std::min(_scrollpos + step, maxscroll(dest));
	paint(dest);
}

void Editor::page_up(WINDOW *dest)
{
	View view(dest);
	size_t step = view.height() / 2;
	_scrollpos = std::max((int)_scrollpos - (int)step, 0);
	paint(dest);
}

size_t Editor::maxscroll(WINDOW *dest)
{
	View view(dest);
	if (_lines.size() > (size_t)view.height()) {
		return _lines.size() - view.height();
	} else {
		return 0;
	}
}
