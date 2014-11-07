#ifndef BROWSER_BROWSER_H
#define BROWSER_BROWSER_H

#include "listform.h"
#include "dirtree.h"
#include "projectlist.h"
#include <memory>

class Browser : public ListForm::Controller, private ProjectList::Delegate
{
	typedef ListForm::Controller inherited;
public:
	Browser();
	virtual void open(Context &ctx) override;
	virtual bool process(Context &ctx, int ch) override;
	void show_projects(Context &ctx);
	virtual void open_project(std::string path) override;
protected:
	virtual void render(ListForm::Builder &lines) override;
private:
	ProjectList _repos;
	std::unique_ptr<DirTree::Root> _project;
};

#endif // BROWSER_BROWSER_H
