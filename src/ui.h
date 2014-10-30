#ifndef UI_H
#define UI_H

#include <ncurses.h>
#include <panel.h>
#include <memory>
#include <vector>

class Window
{
public:
	class Controller
	{
	public:
		virtual ~Controller() = default;
		virtual bool process(Window &window, int ch) = 0;
		virtual std::string title() const = 0;
	};
	Window(std::unique_ptr<Controller> &&controller, int height, int width);
	~Window();
	void move_to(int ypos, int xpos);
	void resize(int height, int width);
	void set_focus();
	bool process(int ch) { return _controller->process(*this, ch); }
	std::string title() const { return _controller->title(); }
protected:
	std::unique_ptr<Controller> _controller;
	WINDOW *_window = nullptr;
	PANEL *_panel = nullptr;
	int _xpos = 0;
	int _ypos = 0;
};

class UI
{
public:
	UI();
	~UI();
	bool process(int ch);
	void open_window(std::unique_ptr<Window::Controller> &&wincontrol);
protected:
	// change the focus to a specific window
	void set_focus(size_t index);
	// reposition all the windows after create/remove/resize
	void relayout();
	// redraw the title bar line at the top
	void drawtitlebar();
	// truncate a window title bar to some length, with padding
	std::string preptitle(std::string title, int barwidth);
private:
	int _width = 0;
	int _height = 0;
	std::vector<std::unique_ptr<Window>> _columns;
	int _spacing = 0;
	size_t _focus = 0;
};

#endif // UI_H
