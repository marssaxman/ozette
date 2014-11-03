#ifndef TREEVIEW_H
#define TREEVIEW_H

#include "listform.h"
#include "browser.h"

class TreeView : public ListForm
{
public:
	TreeView(Browser &host, std::string path);
	virtual std::string title() const { return _dirpath; }
private:
	class CloseRepoField : public Field
	{
	public:
		CloseRepoField(Browser &host): _host(host) {}
		virtual std::string text() const override { return "Switch Repository"; }
		virtual void invoke() override;
	private:
		Browser &_host;
	};
	class EntryField : public Field
	{
	public:
		EntryField(std::string text, std::string path): _text(text), _path(path) {}
		virtual std::string text() const override { return _text; }
		virtual void invoke() override {}
	private:
		std::string _text;
		std::string _path;
	};

	Browser &_host;
	std::string _dirpath;
	void enumerate(std::string path, unsigned indent);
	void subdir(std::string name, std::string path, unsigned indent);
	void subfile(std::string name, std::string path, unsigned indent);
	std::string tab(unsigned indent);
};

#endif	//TREEVIEW_H
