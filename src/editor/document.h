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
	// Locate the position on screen for this character.
	position_t position(const location_t &in_document);
	// Locate the character corresponding to this position.
	location_t location(const position_t &on_display);
	// Retrieve the raw character data for this range.
	std::string text(Range chars);

	// Remove the text within the range.
	void erase(Range chars);
	// Insert this characters at a specific place,
	// returning the end of the inserted text.
	location_t insert(location_t loc, char ch);
private:
	std::unique_ptr<Line> _blank;
	std::vector<std::unique_ptr<Line>> _lines;
	line_t _maxline = 0;	// ubound, not size
};
} // namespace Editor

#endif // EDITOR_DOCUMENT_H
