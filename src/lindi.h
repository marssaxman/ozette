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
	virtual std::string current_dir() const override;
	virtual void edit_file(std::string path) override;
	virtual void close_file(std::string path) override;
	virtual void quit() override { _done = true; }
	virtual void set_clipboard(std::string text) override;
	virtual std::string get_clipboard() override;

	void run();
private:
	void change_directory();
	void new_file();
	void activate_browser();
	int fix_control_quirks(int ch);

	UI::Shell _shell;
	Browser *_browser = nullptr;;
	UI::Window *_browserwindow = nullptr;
	std::string _home_dir;
	std::string _current_dir;
	std::map<std::string, UI::Window*> _editors;
	std::string _clipboard;
	bool _done = false;
};

#endif	//LINDI_H
