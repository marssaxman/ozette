#ifndef UI_H
#define UI_H

#include <ncurses.h>
#include <panel.h>
#include <memory>
#include <vector>

class Window
{
public:
	virtual ~Window();
	Window(int height, int width);
	void move_to(int ypos, int xpos);
	virtual bool process(int ch) = 0;
	virtual std::string title() const = 0;
protected:
	WINDOW *_window = nullptr;
	PANEL *_panel = nullptr;
};

class Console : public Window
{
public:
	Console(int height, int width);
	virtual bool process(int ch) override;
	virtual std::string title() const override { return "Console"; }
};

class Browser : public Window
{
public:
	Browser(int height, int width);
	virtual bool process(int ch) override;
	virtual std::string title() const override { return "Browser"; }
};

class UI
{
public:
	UI();
	~UI();
	bool process(int ch) { return false; }
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
