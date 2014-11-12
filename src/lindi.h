#ifndef LINDI_H
#define LINDI_H

#include "app.h"
#include "shell.h"
#include "browser.h"
#include "paths.h"
#include <string>
#include <vector>
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
	virtual void get_config(std::string name, std::vector<std::string> &lines) override;
	virtual void set_config(std::string name, const std::vector<std::string> &lines) override;

	void run();
private:
	void show_browser();
	void change_directory();
	void new_file();
	int fix_control_quirks(int ch);
	static void set_mru(std::string path, std::vector<std::string> &mru);

	UI::Shell _shell;
	Paths _paths;
	std::map<std::string, UI::Window*> _editors;
	std::string _clipboard;
	bool _done = false;
	std::vector<std::string> _recent_dirs;
};

#endif	//LINDI_H
