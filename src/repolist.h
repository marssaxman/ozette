#ifndef REPOLIST_H
#define REPOLIST_H

#include "browser.h"
#include "listform.h"
#include <vector>

class RepoList : public ListForm
{
public:
	RepoList(Browser &host);
	virtual std::string title() const override { return ""; }
protected:
	virtual void render(Builder &fields) override;
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

	// When we pick a repository, create a repo viewer,
	// then tell the browser to use it as the new delegate.
	Browser &_host;
	// User's home directory, where we expect to find repositories.
	std::string _homedir;
	// These are the repositories we currently know about.
	std::vector<repo_t> _repos;
};

#endif // REPOLIST_H
