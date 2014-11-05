#ifndef EDITOR_COORDINATES_H
#define EDITOR_COORDINATES_H

namespace Editor {

// Documents are a densely-packed ascending sequence of lines.
// Each line consists of a sequence of characters.
// A byte offset locates the beginning of such a character.
typedef size_t line_t;
typedef size_t offset_t;
struct location_t {
	line_t line;
	offset_t offset;
};

// A display space is a two-dimensional plane made up of
// equally sized cells organized in rows and columns.
typedef unsigned row_t;
typedef unsigned column_t;
struct position_t {
	row_t v;
	column_t h;
};

} // namespace Editor

#endif // EDITOR_COORDINATES_H

