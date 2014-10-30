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
	void move_to(int xpos);
	void resize(int height, int width);
	void set_focus();
	void clear_focus();
	bool process(int ch) { return _controller->process(*this, ch); }
protected:
	void draw_title();
private:
	std::unique_ptr<Controller> _controller;
	WINDOW *_window = nullptr;
	PANEL *_panel = nullptr;
	int _xpos = 0;
	bool _has_focus = false;
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
	// compute the desired X position for a given column
	int column_left(size_t index);
private:
	int _width = 0;
	int _height = 0;
	std::vector<std::unique_ptr<Window>> _columns;
	int _spacing = 0;
	size_t _focus = 0;
};

#endif // UI_H
