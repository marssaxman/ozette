#ifndef EDITOR_LINE_H
#define EDITOR_LINE_H

#include <string>
#include <ncurses.h>
#include "coordinates.h"

namespace Editor {
extern const unsigned kTabWidth;
class Line
{
public:
	virtual ~Line() = default;
	// Unformatted bytes
	virtual std::string text() const = 0;
	// Number of bytes present
	virtual size_t size() const;
	// Expected total width of rendered text
	virtual unsigned width();
	// Get display column for some char offset
	virtual column_t column(offset_t loc);
	// Get char offset for some display column
	virtual offset_t offset(column_t h);
	// Render characters from location in buffer
	virtual void paint(WINDOW *view, unsigned width);
private:
	void advance(char ch, column_t &h);
};
} // namespace Editor

#endif // EDITOR_LINE_H
