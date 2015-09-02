//
// ozette
// Copyright (C) 2014-2015 Mars J. Saxman
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
#include <memory>
#include <vector>
#include "editor/coordinates.h"
#include "editor/displayline.h"
#include "editor/changelist.h"
#include "editor/settings.h"

// A document breaks a text buffer into lines, then maps those lines onto an
// infinite plane of equally sized character cells.
namespace Editor {
class Document
{
public:
	Document(const Config &config);
	Document(std::string path, const Config &config);
	void Write(std::string path);
	void View(std::string text);
	std::string status() const { return _status; }
	bool modified() const { return _modified; }
	bool readonly() const { return _read_only; }
	bool can_undo() const { return _edits.can_undo(); }
	bool can_redo() const { return _edits.can_redo(); }
	location_t undo(Update &update) { return _edits.undo(*this, update); }
	location_t redo(Update &update) { return _edits.redo(*this, update); }
	void commit() { _edits.commit(); }

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
	Range find(std::string text, location_t begin);

	// Get the raw text of the indexed source line.
	const std::string &line(line_t index) const;
	// Get a display version of the indexed source line.
	DisplayLine display(line_t index) const;
	// Retrieve the text within the range as a contiguous string.
	std::string text(const Range &span) const;

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
	std::string substr_to_end(const location_t &loc) const;
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

	// Editor settings for the current working directory
	Settings _settings;
	// Syntax for this document's file type
	Syntax::Grammar _syntax;

	std::string _blank;
	std::vector<std::string> _lines;
	line_t _maxline = 0;	// ubound, not size

	// is the user allowed to make changes in this document?
	bool _read_only = false;
	// has the document been edited since it was last read?
	bool _modified = false;
	// what is our user-friendly summary of the file state?
	std::string _status;
	// record of all the edits made to this document
	ChangeList _edits;
};
} // namespace Editor

#endif // EDITOR_DOCUMENT_H
