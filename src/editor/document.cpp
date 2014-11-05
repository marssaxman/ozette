#include "document.h"
#include <fstream>

const unsigned Editor::Document::kTabWidth = 4;

Editor::Document::Document(std::string targetpath)
{
	std::string str;
	std::ifstream file(targetpath);
	while (std::getline(file, str)) {
		_lines.push_back(str);
	}
}

std::string Editor::Document::get_line_text(line_t index) const
{
	// get the specified line, if it is in range, or the empty string
	// if that is what we should display instead
	return index < _lines.size() ? _lines[index] : "";
}

size_t Editor::Document::get_line_size(line_t index) const
{
	// return the line's length in chars, or 0 if out of range
	return index < _lines.size() ? _lines[index].size() : 0;
}

Editor::Document::offset_t Editor::Document::char_for_column(column_t h, line_t index) const
{
	// Given a screen column coordinate and a line number,
	// compute the character offset which most closely
	// precedes that column.
	std::string text = get_line_text(index);
	offset_t charoff = 0;
	column_t xpos = 0;
	for (auto ch: text) {
		xpos += char_width(ch, xpos);
		if (xpos >= h) break;
		charoff++;
	}
	return charoff;
}

Editor::Document::column_t Editor::Document::column_for_char(offset_t charoff, line_t index) const
{
	// Given a character offset and a line number, compute
	// the screen column coordinate where that character
	// should appear.
	std::string text = get_line_text(index);
	charoff = std::min(charoff, text.size());
	column_t column = 0;
	offset_t charpos = 0;
	for (auto ch: text) {
		charpos++;
		if (charpos >= charoff) break;
		column += char_width(ch, column);
	}
	return column;
}

unsigned Editor::Document::char_width(char ch, column_t column) const
{
	// How many columns wide is this character, when
	// it appears at the specified column?
	return (ch == '\t') ? tab_width(column) : 1;
}

unsigned Editor::Document::tab_width(column_t column) const
{
	// How many columns wide is a tab character
	// which begins at the specified column?
	return (column + kTabWidth) % kTabWidth;
}

