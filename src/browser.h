#ifndef BROWSER_H
#define BROWSER_H

#include "controller.h"
#include <memory>

class Browser : public Controller
{
public:
	Browser();
	virtual void paint(WINDOW *view) override;
	virtual bool process(WINDOW *view, int ch) override;
	virtual bool poll(WINDOW *view) override;
	virtual std::string title() const override;
	void set_project(std::string path);
	void close_project();
private:
	// The browser operates in different modes.
	// Each mode is provided by a different controller.
	// Since the window is bound to a single controller, the
	// browser will delegate to the current subcontroller, which
	// can replace itself as the browser's target.
	std::unique_ptr<Controller> _sub;
};

#endif // BROWSER_H
