#ifndef UI_VIEW_H
#define UI_VIEW_H

#include <ncurses.h>
#include "frame.h"

namespace UI {
class View
{
public:
	virtual ~View() = default;
	virtual void activate(Frame &ctx) = 0;
	virtual void deactivate(Frame &ctx) = 0;
	virtual void paint(WINDOW *view, bool active) = 0;
	virtual bool process(Frame &ctx, int ch) = 0;
};
} // namespace UI

#endif // UI_VIEW_H
