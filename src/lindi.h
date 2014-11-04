#ifndef LINDI_H
#define LINDI_H

#include "app.h"
#include "ui.h"
#include "browser.h"
#include <string>
#include <map>

class Lindi : public App, private UI::Delegate
{
public:
	Lindi();
	virtual void edit_file(std::string path) override;
	virtual void select_project(std::string path) override;
	virtual void quit() override { _done = true; }
	void run();
private:
	virtual void window_closed(std::unique_ptr<Window> &&window) override;
	UI _ui;
	Browser *_browser;
	std::map<std::string, Window*> _editors;
	bool _done = false;
};

#endif	//LINDI_H
