#include "editor.h"
#include <fstream>

Editor::Editor(std::string targetpath):
	_targetpath(targetpath)
{
	std::string str;
	std::ifstream file(targetpath);
	if(std::getline(file, str, '\0')) {
		_text = str;
	}
}

void Editor::paint(View &view)
{
	view.fill(_text);
}

bool Editor::process(View &view, int ch)
{
	return false;
}

