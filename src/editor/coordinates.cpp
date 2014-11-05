#include "coordinates.h"

bool Editor::range_t::empty() const
{
	return begin == end;
}

void Editor::range_t::reset(location_t loc)
{
	begin = loc;
	end = loc;
}

void Editor::range_t::extend(location_t a, location_t b)
{
	begin = (a < b) ? a : b;
	end = (a > b) ? a : b;
}

