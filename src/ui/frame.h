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
	virtual void repaint() = 0;
	virtual void set_title(std::string text) = 0;
	virtual void set_status(std::string text) = 0;
	virtual void set_help(const Control::Panel &help) = 0;
	virtual void show_dialog(std::unique_ptr<Dialog> &&dlg) = 0;
};
} // namespace UI

#endif // UI_FRAME_H
