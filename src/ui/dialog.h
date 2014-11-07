#ifndef UI_DIALOG_H
#define UI_DIALOG_H

#include <string>
#include <functional>
#include <memory>
#include <ncurses.h>
#include <panel.h>
#include <vector>
#include "control.h"


namespace UI {
// A dialog is a modal input control.
// Use it when you need to get information from the user before
// performing an action. This may be data input or simply a
// confirmation of intent. The pattern is simply that the dialog
// opens and shows a prompt, the user types in it, then either
// cancel the action, thereby dismissing the dialog, or they can
// commit their input and start the action.
class Dialog
{
public:
	// The dialog controls its appearance and lifetime on screen,
	// but the context allows the dialog controller to manage
	// user input in whatever way is appropriate for the action.
	struct State
	{
		std::string prompt;
		std::string placeholder;
		std::vector<std::string> suggestions;
	};

	// The dialog class manages an interface for interacting with
	// some action, which is implemented by the controller.
	class Controller
	{
	public:
		virtual ~Controller() = default;
		// Set up the starting dialog state and return the
		// starting value for the field. If the starting value is
		// empty, but the list of suggestions is not empty, the
		// cursor will be positioned on the first suggestion and
		// that will be the initial value instead.
		virtual std::string open(State &state) = 0;
		// The user has changed the value in the field.
		// Update dialog attributes if necessary.
		virtual void update(std::string value, State &state) {}
		// The user has pressed tab and wants to fill in the
		// rest of whatever value they have begun to enter.
		virtual std::string autofill(std::string value, State &state)
		{
			return value;
		}
		// The user is happy with their choice and wants
		// the action to proceed. The dialog will close.
		virtual void commit(std::string value) = 0;
	};

	// Every dialog instance gets a unique controller object, which
	// carries whatever state is necessary should the user commit
	// the requested action.
	Dialog(std::unique_ptr<Controller> &&host);
	~Dialog();

	// Dialogs belong to some UI element, which will manage the
	// location of the window and its activation state. This may be
	// some window, which will raise and lower the dialog along with
	// its own state, or it may be the UI shell itself, in which case
	// the dialog floats over all the other windows. The dialog does
	// not particularly have to care.
	void layout(WINDOW *overlay);
	void set_focus();
	void clear_focus();
	void bring_forward();

	// The dialog host must supply it with events so that it can
	// run the text editing and determine when it should close.
	// The dialog will return true if it wants to stay open, false
	// if it is ready to be dismissed. The dialog will stay visible
	// until the dialog object is deleted.
	bool process(int ch);

private:
	void paint();
	void update_window_dimensions();
	// We will get these dimensions whenever we are told to update
	// our layout, since we may need to perform internal layouts
	// when our content changes.
	int _host_height = 0;
	int _host_width = 0;
	int _host_v = 0;
	int _host_h = 0;

	WINDOW *_win = nullptr;
	PANEL *_panel = nullptr;
	bool _has_focus = true;
	std::unique_ptr<Controller> _action;
	// All of our information lives in the state object where the
	// host can manipulate it.
	State _state;
	std::string _value;
	size_t _cursor_pos = 0;
	bool _suggestion_selected = false;
	size_t _sugg_item = 0;
};
} // namespace UI

#endif UI_DIALOG_H
