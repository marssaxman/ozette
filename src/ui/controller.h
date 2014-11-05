#ifndef UI_CONTROLLER_H
#define UI_CONTROLLER_H

#include <string>
#include <ncurses.h>
#include "app.h"

namespace UI {
class Controller
{
public:
	class Context {
	public:
		virtual ~Context() = default;
		virtual void repaint() = 0;
		virtual App &app() = 0;
		virtual void set_title(std::string text) = 0;
	};
	virtual ~Controller() = default;
	// The window has just been opened. Initialize it.
	virtual void open(Context &ctx) = 0;
	// Paint the window with the current state for the
	// object this controller manages.
	virtual void paint(WINDOW *view, bool active) = 0;
	// A keypress has occurred. Update model and view.
	// Return true if the process should continue, false if
	// the keypress represented a close action.
	virtual bool process(Context &ctx, int ch) = 0;
};
} // namespace UI

#endif // UI_CONTROLLER_H
