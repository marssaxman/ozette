#ifndef UI_CONTROLLER_H
#define UI_CONTROLLER_H

#include <ncurses.h>
#include "frame.h"

namespace UI {
class Controller
{
public:
	virtual ~Controller() = default;
	// The window has been opened or brought to the front.
	virtual void activate(Frame &ctx) = 0;
	virtual void deactivate(Frame &ctx) = 0;
	// Paint the window with the current state for the object this
	// controller manages.
	virtual void paint(WINDOW *view, bool active) = 0;
	// A keypress has occurred. Update model and view. Return true if the
	// process should continue, false if the keypress represented a close
	// action and the window should be dismissed.
	virtual bool process(Frame &ctx, int ch) = 0;
};
} // namespace UI

#endif // UI_CONTROLLER_H
