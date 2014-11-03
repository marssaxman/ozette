#ifndef REPOLIST_H
#define REPOLIST_H

#include "browser.h"
#include "listform.h"
#include <vector>

class RepoList : public ListForm
{
public:
	RepoList(Browser &host);
	virtual std::string title() const override { return "Lindi"; }
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
	class RepoField : public Field
	{
	public:
		RepoField(Browser &host, const repo_t &target);
		virtual std::string text() const override { return _target.title; }
		virtual void invoke(std::string) override;
	private:
		Browser &_host;
		repo_t _target;
	};

	// When we pick a repository, create a repo viewer,
	// then tell the browser to use it as the new delegate.
	Browser &_host;
	// User's home directory, where we expect to find repositories.
	std::string _homedir;
};

#endif // REPOLIST_H
