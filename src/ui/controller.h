#ifndef UI_CONTROLLER_H
#define UI_CONTROLLER_H

#include <ncurses.h>
#include "frame.h"

namespace UI {
class Controller
{
public:
	virtual ~Controller() = default;
	// The window has just been opened. Initialize it.
	virtual void open(Frame &ctx) = 0;
	// Paint the window with the current state for the
	// object this controller manages.
	virtual void paint(WINDOW *view, bool active) = 0;
	// A keypress has occurred. Update model and view.
	// Return true if the process should continue, false if
	// the keypress represented a close action.
	virtual bool process(Frame &ctx, int ch) = 0;
};
} // namespace UI

#endif // UI_CONTROLLER_H
