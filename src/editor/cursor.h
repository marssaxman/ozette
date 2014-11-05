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
	size_t line() const { return _line; }
	size_t character() const { return _char; }
	unsigned column() const { return _col; }
	void move_up(size_t lines);
	void move_down(size_t lines);
	void move_left();
	void move_right();
	void move_home();
	void move_end();
private:
	Document &_doc;
	Update &_update;
	size_t _line = 0;
	size_t _char = 0;
	unsigned _col = 0;
};
} // namespace Editor

#endif // EDITOR_CURSOR_H
