#ifndef UI_SHELL_H
#define UI_SHELL_H

#include "window.h"
#include <vector>
#include <queue>

namespace UI {
class Shell
{
public:
	Shell(App &app);
	~Shell();
	bool process(int ch);
	Window *open_window(std::unique_ptr<View> &&wincontrol);
	void close_window(Window *window);
	void make_active(Window *window);
	Window *active() const { return _columns[_focus].get(); }
protected:
	void reap();
	// get the terminal width and height, then calculate column width
	void get_screen_size();
	// change the focus to a specific window
	void set_focus(size_t index);
	// reposition all the windows after create/remove/resize
	void relayout();
	// send this char to the focus window
	void send_to_focus(int ch);
	// close the window with this index
	void close_window(size_t index);
private:
	App &_app;
	int _width = 0;
	int _height = 0;
	std::vector<std::unique_ptr<Window>> _columns;
	int _spacing = 0;
	int _columnWidth = 0;
	size_t _focus = 0;
	std::queue<std::unique_ptr<Window>> _doomed;
};
} // namespace UI

#endif // UI_SHELL_H
