#include "editor.h"
#include <fstream>

Editor::Editor(std::string targetpath):
	_targetpath(targetpath)
{
	std::string str;
	std::ifstream file(targetpath);
	while (std::getline(file, str)) {
		_lines.push_back(str);
	}
}

void Editor::paint(View &view)
{
	size_t effscroll = std::min(maxscroll(view), _scrollpos);
	for (int i = 0; i < view.height(); ++i) {
		size_t line = i + effscroll;
		if (line < _lines.size()) {
			view.write_line(i, _lines[line]);
		} else {
			view.clear_line(i);
		}
	}
}

bool Editor::process(View &view, int ch)
{
	switch (ch) {
		case 258: {	// down arrow
			if (_scrollpos < maxscroll(view)) {
				_scrollpos++;
				paint(view);
			}
		} break;
		case 259: {	// up arrow
			size_t minscroll = 0;
			if (_scrollpos > minscroll) {
				_scrollpos--;
				paint(view);
			}
		} break;
		case 338: { 	// page down
			size_t step = view.height() / 2;
			_scrollpos = std::min(_scrollpos + step, maxscroll(view));
			paint(view);
		} break;
		case 339: {	// page up
			size_t step = view.height() / 2;
			_scrollpos = std::max((int)_scrollpos - (int)step, 0);
			paint(view);
		} break;
		default: break;
	}
	return true;
}

size_t Editor::maxscroll(View &view)
{
	return _lines.size() - view.height();
}
