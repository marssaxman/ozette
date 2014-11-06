#ifndef EDITOR_DOCUMENT_H
#define EDITOR_DOCUMENT_H

#include <string>
#include <vector>
#include <memory>
#include "coordinates.h"
#include "line.h"

// A document breaks a text buffer into lines, then
// maps those lines onto an infinite plane of equally
// sized character cells.
namespace Editor {
class Document
{
public:
	Document();
	Document(std::string targetpath);
	// Get a representation of the line at this index.
	// Will not throw; returns blank if out of bounds.
	Line &line(line_t index);
	// Where is the beginning of the document?
	location_t home();
	// Where is the end of the document?
	location_t end();
	// Which is the last line in the document?
	line_t maxline() const { return _maxline; }

	// Convert back and forth between document and screen coordinates.
	position_t position(const location_t &in_document);
	location_t location(const position_t &on_display);

	// Where is the character which follows or precedes this one?
	location_t next(location_t loc);
	location_t prev(location_t loc);

	// Retrieve the text within the range as a contiguous string.
	std::string text(const Range &span);
	// Remove the text within the range.
	location_t erase(const Range &span);
	// Insert one or more characters at a specific place,
	// returning the end of the inserted text.
	location_t insert(location_t loc, char ch);
	location_t insert(location_t loc, std::string text);
	// Split this character's line in half, returning its position at
	// the beginning of the newly-created following line.
	location_t split(location_t loc);
private:
	std::string substr_to_end(const location_t &loc);
	std::string substr_from_home(const location_t &loc);
	void update_line(line_t index, std::string text);
	void insert_line(line_t index, std::string text);
	void push_to_line(line_t index, std::string prefix);
	void append_to_line(line_t index, std::string suffix);
	line_t append_line(std::string text);
	void sanitize(location_t *loc);
	location_t sanitize(const location_t &loc);
	std::unique_ptr<Line> _blank;
	std::vector<std::unique_ptr<Line>> _lines;
	line_t _maxline = 0;	// ubound, not size
};
} // namespace Editor

#endif // EDITOR_DOCUMENT_H
