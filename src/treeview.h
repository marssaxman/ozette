#ifndef TREEVIEW_H
#define TREEVIEW_H

#include "listform.h"
#include "browser.h"

class TreeView : public ListForm
{
public:
	TreeView(Browser &host, std::string path);
	virtual std::string title() const { return _dirpath; }
protected:
	virtual void render(Fields &fields);
private:
	struct entry
	{
		entry(std::string t, std::string p): text(t), path(p) {}
		std::string text;
		std::string path;
	};
	std::vector<entry> _entries;

	void enumerate(std::string path, unsigned indent);
	void subdir(std::string name, std::string path, unsigned indent);
	void subfile(std::string name, std::string path, unsigned indent);
	std::string tab(unsigned indent);
	void switchrepo();
	Browser &_host;
	std::string _dirpath;
};

#endif	//TREEVIEW_H
