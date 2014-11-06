#ifndef UI_DIALOG_H
#define UI_DIALOG_H

#include <functional>
#include <memory>
#include <ncurses.h>
#include <panel.h>
#include "control.h"

// A dialog is a little widget which overlays a content window and
// requires the user to enter some data before they commit whatever
// action they just invoked. They may also cancel out instead.
// Dialogs may be free-text fields, file path specifiers, or multiple
// choice yes/no/cancel type alert boxes.

namespace UI {
class Dialog
{
public:
	class Controller
	{
	public:
		virtual ~Controller() = default;
		// Call open when the dialog is first shown. It may choose
		// to supply its own help text for the action bar. It must
		// return some reasonable prompt string describing the data
		// we are expecting the user to enter.
		virtual std::string open(Control::Panel &help) = 0;
		// Return true if the dialog needs to stay open, false if
		// it is finished and ready to close up.
		virtual bool process(int ch) = 0;
		// What is the current field value the user is entering?
		virtual std::string value() { return ""; }
		// What is the character location within that string where
		// the cursor should be located?
		virtual size_t cursor() { return 0; }
	};
	Dialog(std::unique_ptr<Controller> &&host);
	~Dialog();
	// Adjust the dialog's size and position. The rectangle given
	// is that of the window's content region. The dialog may occupy
	// as much of this area as it likes.
	void layout(int v, int h, int height, int width);
	void set_focus();
	void clear_focus();
	void bring_forward();
	// Process an event. Return true if the dialog has more work
	// to do, false if it is finished and ready to be dismissed.
	bool process(int ch);
protected:
	void paint();
private:
	WINDOW *_win = nullptr;
	PANEL *_panel = nullptr;
	bool _has_focus = true;
	std::unique_ptr<Controller> _host;
	std::string _prompt;
	size_t _cursor = 0;
};
} // namespace UI

#endif UI_DIALOG_H
