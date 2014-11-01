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
	size_t maxscroll = _lines.size() - view.height();
	size_t effscroll = std::min(maxscroll, _scrollpos);
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
		case 258:	// down arrow
			_scrollpos++;
			paint(view);
		break;
		case 259:	// up arrow
			if (_scrollpos > 0) {
				_scrollpos--;
				paint(view);
			}
		break;
		default: break;
	}
	return true;
}

