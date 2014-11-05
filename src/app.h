#ifndef APP_H
#define APP_H

#include <string>

// Abstract interface for centralized application actions.
class App
{
public:
	enum class Command {
		None = 0,
		Save, // ^S
		Revert, // ^R
		Close, // ^W
		Find, // ^F
		FindNext, // ^G
		Jump, // ^J
		Cut, // ^X
		Copy, // ^C
		Paste, // ^V
		Delete, // ^K
		Undo, // ^Z
		Redo, // ^D
		Open, // ^O
		Project, // ^P
		Help, // ^H   (or ?)
		Config, // ^U
		// consider:
		// Save As
		// Diff, Log, Blame
		// Open Console
		// Switch to Header/Source
		// Create, Move, Rename, Delete File
	};
	virtual ~App() = default;
	virtual void edit_file(std::string path) = 0;
	virtual void file_closed(std::string path) = 0;
	virtual void select_project(std::string path) = 0;
	virtual void quit() = 0;
};

struct help_label_t {
	char key;
	char text[10];
};

struct help_panel_t {
	static const size_t kRows = 2;
	static const size_t kColumns = 6;
	help_label_t label[kRows][kColumns];
};

#endif	//APP_H
