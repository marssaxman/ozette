//
// lindi
// Copyright (C) 2014 Mars J. Saxman
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//

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
	Document(std::string path);
	void Read(std::string path);
	void Write(std::string path);
	void View(std::string text);
	std::string status() const { return _status; }
	bool modified() const { return _modified; }
	bool readonly() const { return _read_only; }

	// Where are the beginning and end of the document?
	location_t home();
	location_t end();
	// Where are the beginning and end of this specific line?
	location_t home(line_t index);
	location_t end(line_t index);
	// Which is the last valid line index in the document?
	line_t maxline() const { return _maxline; }

	// Convert back and forth between document and screen coordinates.
	position_t position(const location_t &in_document);
	location_t location(const position_t &on_display);

	// Where is the character which follows or precedes this one?
	location_t next(location_t loc);
	location_t prev(location_t loc);
	// Where is the next occurrence of the specified string?
	location_t find(std::string text, location_t begin);

	// Get a reference to a specific line, which may be a blank
	// if no such line exists.
	Line &line(line_t index);
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
	bool attempt_modify();
	void clear_modify();

	Line _blank;
	std::vector<std::unique_ptr<Line>> _lines;
	line_t _maxline = 0;	// ubound, not size

	// is the user allowed to make changes in this document?
	bool _read_only = false;
	// has the document been edited since it was last read?
	bool _modified = false;
	// what was the mtime when we last read the file in?
	time_t _last_mtime;
	// what is our user-friendly summary of the file state?
	std::string _status;
};
} // namespace Editor

#endif // EDITOR_DOCUMENT_H
