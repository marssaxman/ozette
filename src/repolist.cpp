#include "repolist.h"
#include <dirent.h>
#include <sys/stat.h>

// List all of the repositories we know about.
// This will be all of the VCS repos in the user's
// home directory.
// We may eventually provide a way to create a new
// repo or to locate one outside the homedir.
// Also list options allowing the user to create a
// new repository or locate one outside ~.

RepoList::RepoList(Browser &host):
	_host(host),
	_homedir(getenv("HOME"))
{
	// Iterate through the directories in the homedir.
	// Check each one to see if it is a VCS directory.
	// If it is, add its path to our list.
	_entry_label = "Repositories:";
	DIR *pdir = opendir(_homedir.c_str());
	if (!pdir) return;
	while (dirent *entry = readdir(pdir)) {
		// We are looking only for directories.
		if (entry->d_type != DT_DIR) continue;
		check_dir(entry->d_name);
	}
	closedir(pdir);
}

RepoList::VCS RepoList::dir_repo_type(std::string path)
{
	// is this a path to some repository under version control?
	// return the VCS type, if any, or none if not a repository
	// of a known system.
	if (dir_exists(path + "/.git")) return VCS::git;
	if (dir_exists(path + "/.svn")) return VCS::svn;
	return VCS::none;
}

bool RepoList::dir_exists(std::string path)
{
	struct stat sb;
	return (stat(path.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode));
}

void RepoList::check_dir(std::string name)
{
	std::string path = _homedir + "/" + name;
	VCS type = dir_repo_type(path);
	if (type != VCS::none) {
		repo_t repo;
		repo.title = "  ~/" + name;
		repo.path = path;
		repo.type = type;
		_entries.emplace_back(new RepoField(_host, repo));
	}
}

RepoList::RepoField::RepoField(Browser &host, const repo_t &target):
	_host(host),
	_target(target)
{
}

void RepoList::RepoField::invoke(std::string)
{
	// Create a viewer for this repository, based on its type.
	// Instruct the browser to delegate itself to this viewer.
}
