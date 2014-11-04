#ifndef DIRTREE_H
#define DIRTREE_H

#include "listform.h"

namespace DirTree {

// Every item in the directory tree is a node.
class Node : public ListForm::Source
{
public:
	Node(std::string path, unsigned indent);
	virtual ~Node() = default;
	std::string path() const { return _path; }
	unsigned indent() const { return _indent; }
private:
	std::string _path;
	unsigned _indent = 0;
};

// A directory is a node which contains other items.
class Directory : public Node
{
public:
	Directory(std::string path, unsigned indent, unsigned subindent);
protected:
	std::vector<std::unique_ptr<Node>> _items;
};

// The root directory is the beginning of our search.
class Root : public Directory
{
public:
	Root(std::string path);
	virtual void render(ListForm::Builder &fields) override;
};

// A branch is a directory which lives inside another.
class Branch : public Directory
{
public:
	Branch(std::string name, std::string path, unsigned indent);
	virtual void render(ListForm::Builder &fields) override;
	std::string name() const { return _name; }
	void toggle() { _open = !_open; }
	bool is_open() const { return _open; }
private:
	std::string _name;
	bool _open = false;
};

// Files are the editable units of the source tree.
class File : public Node
{
public:
	File(std::string name, std::string path, unsigned indent);
	virtual void render(ListForm::Builder &fields) override;
	std::string name() const { return _name; }
private:
	std::string _name;
};

} // namespace DirTree

#endif	//DIRTREE_H
