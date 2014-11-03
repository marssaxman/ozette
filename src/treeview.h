#ifndef TREEVIEW_H
#define TREEVIEW_H

#include "listform.h"
#include "browser.h"
#include "dirtree.h"

class TreeView : public ListForm
{
public:
	TreeView(Browser &host, std::string path);
	virtual std::string title() const { return _path; }
protected:
	virtual void render(Builder &fields);
private:
	void switchrepo();
	Browser &_host;
	std::string _path;
	DirTree::Root _tree;
};

#endif	//TREEVIEW_H
