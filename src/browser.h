#ifndef BROWSER_H
#define BROWSER_H

#include "listform.h"
#include "dirtree.h"
#include "projectlist.h"
#include <memory>

class Browser : public ListForm, private ProjectList::Delegate
{
public:
	Browser();
	virtual std::string title() const override { return "Lindi"; }
protected:
	virtual void render(ListForm::Builder &lines) override;
private:
	virtual void open_project(std::string path) override;
	ProjectList _repos;
	std::unique_ptr<DirTree::Root> _project;
};

#endif // BROWSER_H
