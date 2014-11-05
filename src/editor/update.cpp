#include "update.h"

void Editor::Update::reset()
{
	_dirty = false;
	_linestart = 0;
	_lineend = 0;
}

void Editor::Update::all()
{
	_dirty = true;
	_linestart = 0;
	_lineend = (size_t)-1;
}

void Editor::Update::line(size_t line)
{
	_linestart = _dirty ? std::min(_linestart, line) : line;
	_lineend = _dirty ? std::max(_lineend, line) : line;
	_dirty = true;
}

void Editor::Update::range(size_t a, size_t b)
{
	size_t from = std::min(a, b);
	size_t to = std::max(a, b);
	_linestart = _dirty ? std::min(_linestart, from) : from;
	_lineend = _dirty ? std::max(_linestart, to) : to;
	_dirty = true;
}

bool Editor::Update::is_dirty(size_t line) const
{
	return _dirty && (line >= _linestart) && (line <= _lineend);
}
