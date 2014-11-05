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
	};
	virtual ~Controller() = default;
	// A view has been created or has been cleared.
	// Fill it as necessary based on current model state.
	virtual void paint(WINDOW *view, bool active) = 0;
	// A keypress has occurred. Update model and view.
	// Return true if the process should continue, false if
	// the keypress represented a close action.
	virtual bool process(Context &ctx, int ch, App &app) = 0;
	// Human-readable string identifying the object.
	virtual std::string title() const = 0;
};
} // namespace UI

#endif // UI_CONTROLLER_H
