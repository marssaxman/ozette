#ifndef PROJECTLIST_H
#define PROJECTLIST_H

#include "listform.h"
#include <vector>

class ProjectList : public ListForm::Source
{
public:
	class Delegate
	{
	public:
		virtual ~Delegate() = default;
		virtual void open_project(std::string path) = 0;
	};
	ProjectList(Delegate &host);
	virtual void render(ListForm::Builder &fields) override;
	void toggle() { _open = !_open; }
	void hide_projects() { _open = false; }
	bool are_projects_open() const { return _open; }
	enum class VCS {
		none = 0,
		git = 1,
		svn = 2
	};
	struct repo_t {
		std::string title;
		std::string path;
		VCS type;
	};
	void open_project(const repo_t &repo);
private:
	static VCS dir_repo_type(std::string path);
	static bool dir_exists(std::string path);
	void check_dir(std::string name);

	Delegate &_host;
	// User's home directory, where we expect to find repositories.
	std::string _homedir;
	// These are the repositories we currently know about.
	std::vector<repo_t> _repos;
	// Are we currently displaying the list of projects?
	bool _open = false;
	// Which project did we open last?
	std::string _last_project;
};

#endif // PROJECTLIST_H
