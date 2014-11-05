#ifndef UI_SHELL_H
#define UI_SHELL_H

#include "window.h"
#include <vector>

namespace UI {
class Shell
{
public:
	Shell(App &app);
	~Shell();
	bool process(int ch);
	Window *open_window(std::unique_ptr<Controller> &&wincontrol);
	void make_active(Window *window);
protected:
	// get the terminal width and height, then calculate column width
	void get_screen_size();
	// change the focus to a specific window
	void set_focus(size_t index);
	// reposition all the windows after create/remove/resize
	void relayout();
	// send this char to the focus window
	void send_to_focus(int ch);
	// close the window with this index & tell our delegate
	void close_window(size_t index);
private:
	App &_app;
	int _width = 0;
	int _height = 0;
	std::vector<std::unique_ptr<Window>> _columns;
	int _spacing = 0;
	int _columnWidth = 0;
	size_t _focus = 0;
};
} // namespace UI

#endif // UI_SHELL_H
