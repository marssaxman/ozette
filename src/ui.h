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
	bool process(int ch) { return _controller->process(*this, ch); }
	std::string title() const { return _controller->title(); }
protected:
	std::unique_ptr<Controller> _controller;
	WINDOW *_window = nullptr;
	PANEL *_panel = nullptr;
};

class UI
{
public:
	UI();
	~UI();
	bool process(int ch) { return false; }
	void open(std::unique_ptr<Window::Controller> &&wincontrol);
protected:
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
