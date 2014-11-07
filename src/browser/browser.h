#ifndef BROWSER_BROWSER_H
#define BROWSER_BROWSER_H

#include "listform.h"
#include "dirtree.h"
#include <memory>

class Browser : public ListForm::Controller
{
	typedef ListForm::Controller inherited;
public:
	Browser();
	virtual void activate(UI::Frame &ctx) override;
	virtual bool process(UI::Frame &ctx, int ch) override;
	void show_projects(UI::Frame &ctx);
private:
	void set_title(UI::Frame &ctx);
	virtual void render(ListForm::Builder &lines) override;
	void select_project(UI::Frame &ctx, std::string path);
	void find_projects();
	static bool dir_exists(std::string path);
	static bool file_exists(std::string path);
	std::string _homedir;
	std::vector<std::string> _projects;
	std::unique_ptr<DirTree::Root> _project;
};

#endif // BROWSER_BROWSER_H
