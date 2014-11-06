#ifndef APP_H
#define APP_H

#include <string>

// Abstract interface for centralized application actions.
class App
{
public:
	virtual ~App() = default;
	virtual void edit_file(std::string path) = 0;
	virtual void file_closed(std::string path) = 0;
	virtual void select_project(std::string path) = 0;
	virtual void quit() = 0;
	virtual void set_clipboard(std::string text) = 0;
	virtual std::string get_clipboard() = 0;
};

#endif	//APP_H
