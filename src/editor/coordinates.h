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
	location_t() {}
	location_t(line_t l, offset_t o): line(l), offset(o) {}
	line_t line = 0;
	offset_t offset = 0;
};

// A location range identifies some sequence of characters.
class Range {
public:
	Range() {}
	Range(const location_t &a, const location_t &b);
	const location_t &begin() const { return _begin; }
	const location_t &end() const { return _end; }
	bool empty() const;
	bool multiline() const;
	void reset(const location_t &loc);
	void extend(const location_t &a, const location_t &b);
private:
	location_t _begin;
	location_t _end;
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

