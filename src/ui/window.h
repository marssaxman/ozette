#ifndef UI_WINDOW_H
#define UI_WINDOW_H

#include <ncurses.h>
#include <panel.h>
#include <memory>
#include <vector>
#include "controller.h"
#include "app.h"
#include "dialog.h"

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
	virtual void set_help(const Control::Panel &help) override;
	virtual void show_dialog(std::unique_ptr<Dialog::Controller> &&host);
	void layout_contentwin();
	void layout_taskbar();
	void layout_dialog();
	void paint();
	void paint_content();
	void paint_chrome();
	void paint_title_bar(int height, int width);
	void paint_left_frame(int height, int width);
	void paint_right_frame(int height, int width);
	void paint_taskbar(int height, int width);
private:
	App &_app;
	std::unique_ptr<Controller> _controller;
	// The frame represents the outer dimensions of the window and
	// contains all the UI chrome.
	WINDOW *_framewin = nullptr;
	PANEL *_framepanel = nullptr;
	// The content window belongs to the controller and represents the
	// data that this window exists to display and manipulate.
	WINDOW *_contentwin = nullptr;
	PANEL *_contentpanel = nullptr;
	// There may be a dialog box overlaid on the content window, if the
	// user is currently engaged in some process which requires input.
	std::unique_ptr<UI::Dialog> _dialog;
	// Are we the active window? This changes the way we draw our chrome.
	bool _has_focus = true;
	// What are the dimensional attributes we worked out during layout?
	bool _lframe = false;
	bool _rframe = false;
	unsigned _taskbar_height = 0;
	// Have we experienced changes which require repainting but which we
	// have not yet had a chance to implement?
	bool _dirty_content = true;
	bool _dirty_chrome = true;
	// Data to display in our window chrome: this is for the use of our
	// controller object, which can update these fields as it pleases.
	std::string _title;
	std::string _status;
	Control::Panel _help;
};
} // namespace UI

#endif // UI_WINDOW_H
