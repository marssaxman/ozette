#include "projectlist.h"
#include <dirent.h>
#include <sys/stat.h>

// The project list is a popup menu listing all of the
// directories we think the user might care about.
// For the time being this is just a list of all the
// VCS repositories in the home directory.

ProjectList::ProjectList(Delegate &host):
	_host(host),
	_homedir(getenv("HOME"))
{
	// Iterate through the directories in the homedir.
	// Check each one to see if it is a VCS directory.
	// If it is, add its path to our list.
	DIR *pdir = opendir(_homedir.c_str());
	if (!pdir) return;
	while (dirent *entry = readdir(pdir)) {
		// We are looking only for directories.
		if (entry->d_type != DT_DIR) continue;
		check_dir(entry->d_name);
	}
	closedir(pdir);
}

void ProjectList::render(ListForm::Builder &fields)
{
	fields.label("Repositories:");
	for (auto &repo: _repos) {
		auto action = [this, repo](){open_repo(repo);};
		fields.entry(repo.title, action);
	}
}

ProjectList::VCS ProjectList::dir_repo_type(std::string path)
{
	// is this a path to some repository under version control?
	// return the VCS type, if any, or none if not a repository
	// of a known system.
	if (dir_exists(path + "/.git")) return VCS::git;
	if (dir_exists(path + "/.svn")) return VCS::svn;
	return VCS::none;
}

bool ProjectList::dir_exists(std::string path)
{
	struct stat sb;
	return (stat(path.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode));
}

void ProjectList::check_dir(std::string name)
{
	std::string path = _homedir + "/" + name;
	VCS type = dir_repo_type(path);
	if (type != VCS::none) {
		repo_t repo;
		repo.title = "  ~/" + name;
		repo.path = path;
		repo.type = type;
		_repos.push_back(repo);
	}
}

void ProjectList::open_repo(const repo_t &target)
{
	_host.open_project(target.path);
}
