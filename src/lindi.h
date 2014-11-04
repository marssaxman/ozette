#ifndef LINDI_H
#define LINDI_H

#include "ui.h"
#include "browser.h"
#include <string>
#include <map>

class Lindi : private UI::Delegate
{
public:
	Lindi();
	void edit_file(std::string path);
	void run();
	void quit();
private:
	virtual void window_closed(std::unique_ptr<Window> &&window) override;
	UI _ui;
	Browser *_browser;
	std::map<std::string, Window*> _editors;
};

#endif	//LINDI_H
