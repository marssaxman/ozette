#include "line.h"

const unsigned Editor::kTabWidth = 4;

size_t Editor::Line::size() const
{
	return text().size();
}

unsigned Editor::Line::width()
{
	return column(size());
}

Editor::column_t Editor::Line::column(offset_t loc)
{
	column_t out = 0;
	offset_t pos = 0;
	for (auto ch: text()) {
		if (++pos > loc) break;
		advance(ch, out);
	}
	return out;
}

Editor::offset_t Editor::Line::offset(column_t h)
{
	offset_t out = 0;
	column_t pos = 0;
	for (char ch: text()) {
		advance(ch, pos);
		if (pos > h) break;
		out++;
	}
	return out;
}

void Editor::Line::advance(char ch, column_t &h)
{
	do {
	h++;
	} while (ch == '\t' && h % kTabWidth);
}
