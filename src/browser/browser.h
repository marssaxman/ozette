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
	void view(std::string path);
private:
	void set_title(UI::Frame &ctx);
	virtual void render(ListForm::Builder &lines) override;
	std::unique_ptr<DirTree::Root> _tree;
};

#endif // BROWSER_BROWSER_H
