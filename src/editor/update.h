#ifndef EDITOR_UPDATE_H
#define EDITOR_UPDATE_H

#include "controller.h"
#include <vector>

namespace Editor {
class Update
{
public:
	void reset();
	void all();
	void line(size_t line);
	void range(size_t from, size_t to);
	bool has_dirty() const { return _dirty; }
	bool is_dirty(size_t line) const;
private:
	bool _dirty = true;
	size_t _linestart = 0;
	size_t _lineend = 0;
};
} // namespace Editor

#endif // EDITOR_UPDATE_H
