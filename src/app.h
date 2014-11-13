#ifndef APP_H
#define APP_H

#include <string>
#include <vector>

// Abstract interface for centralized application actions.
class App
{
public:
	virtual ~App() = default;
	virtual std::string current_dir() const = 0;
	virtual void edit_file(std::string path) = 0;
	virtual void close_file(std::string path) = 0;
	virtual void set_clipboard(std::string text) = 0;
	virtual std::string get_clipboard() = 0;
	virtual void get_config(std::string name, std::vector<std::string> &lines) = 0;
	virtual void set_config(std::string name, const std::vector<std::string> &lines) = 0;
};

#endif	//APP_H
