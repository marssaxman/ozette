#include "update.h"
#include <climits>

void Editor::Update::reset()
{
	_dirty = false;
	_start = 0;
	_end = 0;
}

void Editor::Update::at(location_t loc)
{
	at(loc.line);
}

void Editor::Update::at(line_t index)
{
	_start = _dirty? std::min(_start, index): index;
	_end = _dirty? std::max(_end, index): index;
	_dirty = true;
}

void Editor::Update::range(line_t a, line_t b)
{
	line_t from = std::min(a, b);
	line_t to = std::max(a, b);
	_start = _dirty? std::min(_start, from): from;
	_end = _dirty? std::max(_start, to): to;
	_dirty = true;
}

void Editor::Update::forward(location_t loc)
{
	_start = _dirty? std::min(_start, loc.line): loc.line;
	_end = SIZE_MAX;
	_dirty = true;
}

void Editor::Update::all()
{
	_dirty = true;
	_start = 0;
	_end = SIZE_MAX;
}

bool Editor::Update::is_dirty(line_t index) const
{
	return _dirty && (index >= _start) && (index <= _end);
}
