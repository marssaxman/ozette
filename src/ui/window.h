#ifndef UI_WINDOW_H
#define UI_WINDOW_H

#include <ncurses.h>
#include <panel.h>
#include <memory>
#include <vector>
#include "view.h"
#include "app.h"
#include "dialog.h"
#include "helpbar.h"

namespace UI {
class Window : public Frame
{
public:
	Window(App &app, std::unique_ptr<View> &&controller);
	~Window();
	void layout(int xpos, int width);
	void set_focus();
	void clear_focus();
	void bring_forward(int focus_relative);
	bool process(int ch);
protected:
	virtual void repaint() override { _dirty_content = true; }
	virtual App &app() override { return _app; }
	virtual void set_title(std::string text) override;
	virtual void set_status(std::string text) override;
	virtual void set_help(const Control::Panel &help) override;
	virtual void show_dialog(std::unique_ptr<Dialog> &&dialog) override;
	virtual void show_result(std::string message) override;
	void clear_result();
	void calculate_content(int &vpos, int &hpos, int &height, int &width);
	void layout_contentwin();
	void layout_taskbar();
	void paint();
	void paint_content();
	void paint_chrome();
	void paint_titlebar(int width);
	void paint_titlebar_left(int width, std::string text);
	void paint_titlebar_right(int width, std::string text);
	void paint_left_frame(int height, int width);
	void paint_right_frame(int height, int width);
	void paint_taskbar(int height, int width);
private:
	App &_app;
	std::unique_ptr<View> _view;
	// The frame represents the outer dimensions of the window and
	// contains all the UI chrome.
	WINDOW *_framewin = nullptr;
	PANEL *_framepanel = nullptr;
	// There may be a dialog box overlaid on the content window, if the
	// user is currently engaged in some process which requires input.
	std::unique_ptr<UI::Dialog> _dialog;
	// There may be an ephemeral result box overlaid on the content window.
	WINDOW *_resultwin = nullptr;
	PANEL *_resultpanel = nullptr;
	// Are we the active window? This changes the way we draw our chrome.
	bool _has_focus = false;
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
	bool _swap_titlebar = false;
	HelpBar::Panel _help;
};
} // namespace UI

#endif // UI_WINDOW_H
