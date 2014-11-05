#ifndef EDITOR_UPDATE_H
#define EDITOR_UPDATE_H

#include "controller.h"
#include "coordinates.h"

namespace Editor {
class Update
{
public:
	void reset();
	void at(location_t loc);
	void at(line_t line);
	void range(line_t from, line_t to);
	void all();
	bool has_dirty() const { return _dirty; }
	bool is_dirty(line_t index) const;
private:
	bool _dirty = true;
	line_t _start = 0;
	line_t _end = 0;
};
} // namespace Editor

#endif // EDITOR_UPDATE_H
