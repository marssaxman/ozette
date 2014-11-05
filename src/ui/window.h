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
	void layout(int xpos, int width);
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
	virtual void set_help(const help_panel_t *help) override;
	void layout_contentwin();
	void paint();
	void paint_content();
	void paint_chrome();
	void paint_title_bar(int height, int width);
	void paint_left_frame(int height, int width);
	void paint_right_frame(int height, int width);
	void paint_task_bar(int height, int width);
private:
	App &_app;
	std::unique_ptr<Controller> _controller;
	WINDOW *_framewin = nullptr;
	PANEL *_framepanel = nullptr;
	WINDOW *_contentwin = nullptr;
	PANEL *_contentpanel = nullptr;
	bool _has_focus = true;
	bool _lframe = false;
	bool _rframe = false;
	unsigned _taskbar_height = 0;
	bool _dirty_content = true;
	bool _dirty_chrome = true;
	std::string _title;
	std::string _status;
	const help_panel_t *_help = nullptr;
};
} // namespace UI

#endif // UI_WINDOW_H
