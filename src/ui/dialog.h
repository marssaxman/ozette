#ifndef UI_DIALOG_H
#define UI_DIALOG_H

#include <string>
#include <functional>
#include <memory>
#include <ncurses.h>
#include <panel.h>
#include <vector>
#include "helpbar.h"
#include "view.h"

namespace UI {
// A dialog is a modal input control.
// Use it when you need to get information from the user before
// performing an action. This may be data input or simply a
// confirmation of intent. The pattern is simply that the dialog
// opens and shows a prompt, the user types in it, then either
// cancel the action, thereby dismissing the dialog, or they can
// commit their input and start the action.
class Dialog : public UI::View
{
	typedef UI::View inherited;
public:
	typedef std::function<void(Frame&, std::string)> action_t;
	struct Layout {
		std::string prompt;
		bool show_value = true;
		std::string value;
		std::vector<std::string> options;
		action_t commit = nullptr;
		action_t yes = nullptr;
		action_t no = nullptr;
	};
	Dialog(const Layout &layout);

	// Dialogs belong to some UI element, which will manage the
	// location of the window and its activation state. This may be
	// some window, which will raise and lower the dialog along with
	// its own state, or it may be the UI shell itself, in which case
	// the dialog floats over all the other windows. The dialog does
	// not particularly have to care.
	virtual void layout(int vpos, int hpos, int height, int width) override;
	virtual bool process(UI::Frame &ctx, int ch) override;
	virtual void paint(bool active) override { inherited::paint(active); }
	void set_help(HelpBar::Panel &panel);

protected:
	virtual void paint(WINDOW *view, bool active) override;

private:
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

	// Layout structure supplied by the client.
	Layout _layout;

	// The cursor may be in the edit field or the suggestion list.
	size_t _cursor_pos = 0;
	bool _suggestion_selected = false;
	size_t _sugg_item = 0;
	// Do we need to repaint the window?
	bool _repaint = true;

};
} // namespace UI

#endif UI_DIALOG_H
