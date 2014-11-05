#ifndef UI_WINDOW_H
#define UI_WINDOW_H

#include <ncurses.h>
#include <panel.h>
#include <memory>
#include <vector>
#include "controller.h"
#include "app.h"

namespace UI {
class Window : public Controller::Context
{
public:
	Window(App &app, std::unique_ptr<Controller> &&controller);
	~Window();
	void layout(int xpos, int height, int width, bool lframe, bool rframe);
	void set_focus();
	void clear_focus();
	void bring_forward();
	bool process(int ch);
	bool poll();
protected:
	virtual void repaint() override { _dirty_content = true; }
	virtual App &app() override { return _app; }
	virtual void set_title(std::string text) override;
	virtual void set_status(std::string text) override;
	void paint();
	void paint_chrome();
	void paint_content();
private:
	App &_app;
	std::unique_ptr<Controller> _controller;
	int _xpos = 0;
	int _height = 0;
	int _width = 0;
	WINDOW *_framewin = nullptr;
	PANEL *_framepanel = nullptr;
	WINDOW *_contentwin = nullptr;
	PANEL *_contentpanel = nullptr;
	bool _has_focus = true;
	bool _lframe = false;
	bool _rframe = false;
	unsigned _task_bar_height = 0;
	bool _dirty_content = true;
	bool _dirty_chrome = true;
	std::string _title;
	std::string _status;
};
} // namespace UI

#endif // UI_WINDOW_H
