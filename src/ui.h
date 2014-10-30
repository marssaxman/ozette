#ifndef UI_H
#define UI_H

#include "window.h"
#include <vector>

class UI
{
public:
	UI();
	~UI();
	bool process(int ch);
	void open_window(std::unique_ptr<Window::Controller> &&wincontrol);
protected:
	// get the terminal width and height, then calculate column width
	void get_screen_size();
	// change the focus to a specific window
	void set_focus(size_t index);
	// reposition all the windows after create/remove/resize
	void relayout();
private:
	int _width = 0;
	int _height = 0;
	std::vector<std::unique_ptr<Window>> _columns;
	int _spacing = 0;
	int _columnWidth = 0;
	size_t _focus = 0;
};

#endif // UI_H
