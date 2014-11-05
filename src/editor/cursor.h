#ifndef EDITOR_CURSOR_H
#define EDITOR_CURSOR_H

#include "document.h"
#include "update.h"

// A cursor points at a location in a document and
// navigates to other locations relative to it.
namespace Editor {
class Cursor
{
public:
	Cursor(Document &doc, Update &update);
	const location_t location() const { return _location; }
	const position_t position() const { return _display; }
	void move_up(size_t count);
	void move_down(size_t count);
	void move_left();
	void move_right();
private:
	void begin_move();
	void commit_location();
	void commit_position();
	Document &_doc;
	Update &_update;
	location_t _location = {0,0};
	// We use this internal position record to keep the cursor
	// located on the same column when moving up and down, as
	// closely as we can approximate. The display position of
	// the cursor is a separate value, calculated back from
	// the location, so that it always corresponds to an actual
	// character (whereas the abstract position may point at a
	// cell which is not occupied by any actual character).
	position_t _position = {0,0};
	position_t _display = {0,0};
};
} // namespace Editor

#endif // EDITOR_CURSOR_H
