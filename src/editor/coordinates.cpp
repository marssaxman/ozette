#include "coordinates.h"

Editor::Range::Range(const location_t &a, const location_t &b)
{
	_begin = (a < b) ? a : b;
	_end = (a > b) ? a : b;
}

bool Editor::Range::empty() const
{
	return _begin == _end;
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
