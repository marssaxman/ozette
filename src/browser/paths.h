#ifndef BROWSER_PATHS_H
#define BROWSER_PATHS_H

#include <string>

class Paths
{
public:
	Paths();
	std::string home() const { return _homedir; }
	std::string current() const { return _curdir; }
	std::string config() const { return _configdir; }
	void set_current(std::string dir);
	std::string canonical(std::string path);
	std::string display(std::string path);
private:
	std::string _homedir;
	std::string _curdir;
	std::string _configdir;
};

#endif //BROWSER_PATHS_H
