#ifndef EDITOR_COORDINATES_H
#define EDITOR_COORDINATES_H

#include <cstddef>

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

// A location range identifies some sequence of characters.
struct range_t {
	location_t begin;
	location_t end;
	bool empty() const;
	void reset(location_t loc);
	void extend(location_t a, location_t b);
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

inline bool operator==(const Editor::location_t &lhs, const Editor::location_t &rhs)
{
	return lhs.line == rhs.line && lhs.offset == rhs.offset;
}

inline bool operator!=(const Editor::location_t& lhs, const Editor::location_t& rhs)
{
	return !operator==(lhs,rhs);
}

inline bool operator< (const Editor::location_t& lhs, const Editor::location_t& rhs)
{
	if (lhs.line < rhs.line) return true;
	if (lhs.line > rhs.line) return false;
	return lhs.offset < rhs.offset;
}

inline bool operator> (const Editor::location_t& lhs, const Editor::location_t& rhs)
{
	return  operator< (rhs,lhs);
}

inline bool operator<=(const Editor::location_t& lhs, const Editor::location_t& rhs)
{
	return !operator> (lhs,rhs);
}

inline bool operator>=(const Editor::location_t& lhs, const Editor::location_t& rhs)
{
	return !operator< (lhs,rhs);
}

#endif // EDITOR_COORDINATES_H

