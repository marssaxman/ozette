#ifndef UI_FRAME_H
#define UI_FRAME_H

#include <string>
#include <memory>
#include "app.h"
#include "control.h"

namespace UI {
class Dialog;
// A frame represents the portion of a window managed by the shell.
// This interface allows the window's controller to manipulate those
// elements of the window which refer to its content and not to its
// function within the shell.
class Frame {
public:
	virtual ~Frame() = default;
	virtual App &app() = 0;
	// The window content should be redrawn.
	virtual void repaint() = 0;
	// Change the title text, status text, or help text for the
	// window this controller is managing.
	virtual void set_title(std::string text) = 0;
	virtual void set_status(std::string text) = 0;
	// Open a dialog box and request input from the user.
	// The controller will be suspended while the dialog is open.
	virtual void show_dialog(std::unique_ptr<Dialog> &&dialog) = 0;
	// Some process has completed. Show the result to the user as
	// a temporary floating result bar.
	virtual void show_result(std::string) = 0;
};
} // namespace UI

#endif // UI_FRAME_H
