#include "coordinates.h"

std::string Editor::location_t::to_string() const
{
	return std::to_string(line) + ":" + std::to_string(offset);
}

Editor::Range::Range(const location_t &a, const location_t &b)
{
	_begin = (a < b) ? a : b;
	_end = (a > b) ? a : b;
}

std::string Editor::Range::to_string() const
{
	return _begin.to_string() + "-" + _end.to_string();
}

bool Editor::Range::empty() const
{
	return _begin == _end;
}

bool Editor::Range::multiline() const
{
	return _begin.line != _end.line;
}

void Editor::Range::reset(const location_t &loc)
{
	_begin = loc;
	_end = loc;
}

void Editor::Range::extend(const location_t &a, const location_t &b)
{
	_begin = (a < b) ? a : b;
	_end = (a > b) ? a : b;
}
