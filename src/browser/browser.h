#ifndef BROWSER_BROWSER_H
#define BROWSER_BROWSER_H

#include "listform.h"
#include "dirtree.h"
#include "projectlist.h"
#include <memory>

class Browser : public ListForm::Controller, private ProjectList::Delegate
{
public:
	Browser();
	virtual void open(Context &ctx) override { ctx.set_title("Lindi"); }
	void show_projects();
	virtual void open_project(std::string path) override;
protected:
	virtual void render(ListForm::Builder &lines) override;
private:
	ProjectList _repos;
	std::unique_ptr<DirTree::Root> _project;
};

#endif // BROWSER_BROWSER_H
