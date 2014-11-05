#ifndef EDITOR_DOCUMENT_H
#define EDITOR_DOCUMENT_H

#include <string>
#include <vector>

// A document maps a text buffer into an infinite plane
// of equally-sized character cells. It is responsible for
// interpreting the formatting control characters.
namespace Editor {
class Document
{
public:
	Document(std::string targetpath);
	static const unsigned kTabWidth;
	size_t maxline() const { return _maxline; }
	std::string get_line_text(size_t index) const;
	size_t get_line_size(size_t index) const;
	size_t char_for_column(unsigned column, size_t line) const;
	unsigned column_for_char(size_t charoff, size_t line) const;
	unsigned char_width(char ch, size_t column) const;
	unsigned tab_width(size_t column) const;
private:
	std::vector<std::string> _lines;
	size_t _maxline = 0;	// ubound, not size
};
} // namespace Editor

#endif // EDITOR_DOCUMENT_H
