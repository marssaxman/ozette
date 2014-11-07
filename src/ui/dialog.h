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
class Frame;
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
	// The dialog class manages an interface for interacting with
	// some action which will be performed when the user commits.
	class Action
	{
	public:
		virtual ~Action() = default;
		// The user is happy with their choice and wants
		// the action to proceed. The dialog will close.
		virtual void commit(UI::Frame &ctx, std::string value) = 0;
	};

	// Every dialog instance gets a unique controller object, which
	// carries whatever state is necessary should the user commit
	// the requested action.
	Dialog(std::string prompt, std::string value, std::unique_ptr<Action> &&action);
	Dialog(std::string prompt, std::vector<std::string> values, std::unique_ptr<Action> &&action);
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
	bool process(UI::Frame &ctx, int ch);

private:
	void paint();
	void arrow_left();
	void arrow_right();
	void arrow_up();
	void arrow_down();
	void delete_prev();
	void delete_next();
	void key_insert(int ch);
	void select_suggestion(size_t i);
	void select_field();
	void set_value(std::string val);

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
	std::unique_ptr<Action> _action;
	// Do we need to repaint the window?
	bool _repaint = true;

	std::string _prompt;
	std::string _value;
	std::vector<std::string> _suggestions;

	// The cursor may be in the edit field or the suggestion list.
	size_t _cursor_pos = 0;
	bool _suggestion_selected = false;
	size_t _sugg_item = 0;
};
} // namespace UI

#endif UI_DIALOG_H
