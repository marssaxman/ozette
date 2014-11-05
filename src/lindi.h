#ifndef LINDI_H
#define LINDI_H

#include "app.h"
#include "shell.h"
#include "browser.h"
#include <string>
#include <map>

class Lindi : public App
{
public:
	Lindi();
	virtual void edit_file(std::string path) override;
	virtual void file_closed(std::string path) override;
	virtual void select_project(std::string path) override;
	virtual void quit() override { _done = true; }
	void run();
private:
	UI::Shell _shell;
	Browser *_browser;
	std::map<std::string, UI::Window*> _editors;
	bool _done = false;
};

#endif	//LINDI_H