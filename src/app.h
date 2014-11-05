#ifndef APP_H
#define APP_H

#include <string>

// Abstract interface for centralized application actions.
class App
{
public:
	enum class Command {
		None = 0,
		Save, SaveAs, Close,
		Find, FindNext, JumpTo,
		Cut, Copy, Paste, Delete,
		Undo, Redo,
		Open, Create, Move, Project,
		Help, Config
	};
	virtual ~App() = default;
	virtual void edit_file(std::string path) = 0;
	virtual void file_closed(std::string path) = 0;
	virtual void select_project(std::string path) = 0;
	virtual void quit() = 0;
};

#endif	//APP_H
