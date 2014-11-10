#ifndef UI_VIEW_H
#define UI_VIEW_H

#include <ncurses.h>
#include <panel.h>
#include "frame.h"

namespace UI {
class View
{
public:
	View();
	virtual ~View();
	void layout(int v, int h, int height, int width);
	void bring_forward();
	virtual void paint(bool active);
	virtual void activate(Frame &ctx) = 0;
	virtual void deactivate(Frame &ctx) = 0;
	virtual bool process(Frame &ctx, int ch) = 0;
protected:
	virtual void paint(WINDOW *view, bool active) = 0;
private:
	WINDOW *_window = nullptr;
	PANEL *_panel = nullptr;
};
} // namespace UI

#endif // UI_VIEW_H
