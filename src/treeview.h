#ifndef TREEVIEW_H
#define TREEVIEW_H

#include "listform.h"
#include "browser.h"

class TreeView : public ListForm
{
public:
	TreeView(Browser &host, std::string path);
	virtual std::string title() const { return _path; }
protected:
	virtual void render(Builder &fields);
private:
	// Every item in the directory tree is a node.
	class Node
	{
	public:
		Node(std::string path): _path(path) {}
		virtual ~Node() = default;
		virtual void render(Builder &fields) = 0;
	private:
		std::string _path;
	};
	// A directory is a node which contains other items.
	class Directory : public Node
	{
	public:
		Directory(std::string path);
	protected:
		std::vector<std::unique_ptr<Node>> _items;
	private:
		void subdir(std::string name, std::string path);
		void subfile(std::string name, std::string path);
	};
	// The root directory is the beginning of our search.
	class Root : public Directory
	{
	public:
		Root(std::string path);
		virtual void render(Builder &fields) override;
	};
	// A branch is a directory which lives inside another.
	class Branch : public Directory
	{
	public:
		Branch(std::string name, std::string path);
		virtual void render(Builder &fields) override;
	private:
		std::string _name;
		bool _open = false;
	};
	// Files are the editable units of the source tree.
	class File : public Node
	{
	public:
		File(std::string name, std::string path);
		virtual void render(Builder &fields) override;
	private:
		std::string _name;
	};
	void switchrepo();
	Browser &_host;
	std::string _path;
	Root _dir;
};

#endif	//TREEVIEW_H
