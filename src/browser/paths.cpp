#include "paths.h"
#include <unistd.h>

Paths::Paths():
	_homedir(getenv("HOME")),
	_curdir(get_current_dir_name()),
	_configdir(_homedir + "/.lindi")
{
}

void Paths::set_current(std::string path)
{
	_curdir = path;
}

std::string Paths::canonical(std::string path)
{
	return path;
}

std::string Paths::display(std::string path)
{
	return path;
}

