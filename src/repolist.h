#ifndef REPOLIST_H
#define REPOLIST_H

#include "listform.h"
#include <vector>

class RepoList : public ListForm::Source
{
public:
	class Delegate
	{
	public:
		virtual ~Delegate() = default;
		virtual void open_project(std::string path) = 0;
	};
	RepoList(Delegate &host);
	virtual void render(ListForm::Builder &fields) override;
private:
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
	static VCS dir_repo_type(std::string path);
	static bool dir_exists(std::string path);
	void check_dir(std::string name);
	void open_repo(const repo_t &repo);

	Delegate &_host;
	// User's home directory, where we expect to find repositories.
	std::string _homedir;
	// These are the repositories we currently know about.
	std::vector<repo_t> _repos;
};

#endif // REPOLIST_H
