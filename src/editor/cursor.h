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
	line_t line() const { return _location.line; }
	offset_t character() const { return _location.offset; }
	column_t column() const { return _position.h; }
	void move_up(size_t count);
	void move_down(size_t count);
	void move_left();
	void move_right();
private:
	Document &_doc;
	Update &_update;
	location_t _location = {0,0};
	position_t _position= {0,0};
};
} // namespace Editor

#endif // EDITOR_CURSOR_H
