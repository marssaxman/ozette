#ifndef EDITOR_DOCUMENT_H
#define EDITOR_DOCUMENT_H

#include <string>
#include <vector>

// A document breaks a text buffer into lines, then
// maps those lines onto an infinite plane of equally
// sized character cells.
namespace Editor {
class Document
{
public:
	Document(std::string targetpath);

	// Location within the document
	typedef size_t line_t;
	typedef size_t offset_t;
	struct location_t {
		line_t line;
		offset_t offset;
	};
	// Position on the character plane
	typedef unsigned row_t;
	typedef unsigned column_t;
	struct position_t {
		row_t v;
		column_t h;
	};

	static const unsigned kTabWidth;
	line_t maxline() const { return _maxline; }
	std::string get_line_text(line_t index) const;
	size_t get_line_size(line_t index) const;
	offset_t char_for_column(column_t h, line_t index) const;
	column_t column_for_char(offset_t c, line_t index) const;
	unsigned char_width(char ch, column_t h) const;
	unsigned tab_width(column_t h) const;
private:
	std::vector<std::string> _lines;
	line_t _maxline = 0;	// ubound, not size
};
} // namespace Editor

#endif // EDITOR_DOCUMENT_H
