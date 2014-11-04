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

namespace Menu {
class Field : public ListForm::Field
{
public:
	Field(std::string text, size_t maxlen):
		_text(text),
		_maxlen(maxlen)
	{
	}
	virtual void paint(WINDOW *view, size_t width)
	{
		wattron(view, A_BOLD);
		emitch(view, boxleft(), width);
		wattroff(view, A_BOLD);
		size_t gap = _maxlen - _text.size();
		if (!_text.empty()) {
			emitch(view, ' ', width);
			emitstr(view, _text, width);
			emitch(view, ' ', width);
		} else {
			gap += 2;
		}
		emitrep(view, spacer(), gap, width);
		wattron(view, A_BOLD);
		emitch(view, boxright(), width);
		wattroff(view, A_BOLD);
	}
	virtual void get_highlight(size_t &off, size_t &len) override
	{
		off = 1;
		len = _maxlen + 2;
	}
protected:
	virtual int boxleft() const = 0;
	virtual int spacer() const = 0;
	virtual int boxright() const = 0;
private:
	std::string _text;
	size_t _maxlen;
};

// Closed menu label
class Label : public Field
{
public:
	Label(std::string title, size_t maxlen, ProjectList &list):
		Field(title, maxlen), _list(list) {}
	virtual bool invoke(App &app) override
	{
		_list.toggle();
		return true;
	}
protected:
	virtual int boxleft() const override { return ACS_HLINE; }
	virtual int spacer() const override { return ' '; }
	virtual int boxright() const override { return ACS_HLINE; }
private:
	ProjectList &_list;
};

class Header : public Field
{
public:
	Header(std::string title, size_t maxlen, ProjectList &list):
		Field(title, maxlen), _list(list) {}
	virtual bool invoke() override
	{
		_list.toggle();
		return true;
	}
	virtual bool cancel() override
	{
		_list.toggle();
		return true;
	}
protected:
	virtual int boxleft() const override { return ACS_ULCORNER; }
	virtual int spacer() const override { return ' '; }
	virtual int boxright() const override { return ACS_URCORNER; }
private:
	ProjectList &_list;
};

class Footer : public Field
{
public:
	Footer(size_t maxlen):
		Field("", maxlen) {}
	virtual bool active() const override { return false; }
protected:
	virtual int boxleft() const override { return ACS_LLCORNER; }
	virtual int spacer() const override { return ACS_HLINE; }
	virtual int boxright() const override { return ACS_LRCORNER; }
};

class Item : public Field
{
public:
	Item(ProjectList::repo_t &project, size_t maxlen, ProjectList &list):
		Field(project.title, maxlen), _project(project), _list(list) {}
	virtual bool invoke() override
	{
		_list.open_project(_project);
		return true;
	}
	virtual bool cancel() override
	{
		_list.hide_projects();
		return true;
	}
protected:
	virtual int boxleft() const override { return ACS_VLINE; }
	virtual int spacer() const override { return ' '; }
	virtual int boxright() const override { return ACS_VLINE; }
private:
	ProjectList::repo_t _project;
	ProjectList &_list;
};

} // namespace Menu

void ProjectList::render(ListForm::Builder &fields)
{
	std::string title = "Project:";
	if (!_last_project.empty()) {
		title += " " + _last_project;
	}
	size_t maxlen = title.size();
	for (auto &repo: _repos) {
		maxlen = std::max(maxlen, repo.title.size());
	}

	std::unique_ptr<ListForm::Field> field;
	if (_open) {
		field.reset(new Menu::Header(title, maxlen, *this));
		fields.add(std::move(field));
		for (auto &repo: _repos) {
			field.reset(new Menu::Item(repo, maxlen, *this));
			fields.add(std::move(field));
		}
		field.reset(new Menu::Footer(maxlen));
		fields.add(std::move(field));
	} else {
		field.reset(new Menu::Label(title, maxlen, *this));
		fields.add(std::move(field));
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
		repo.title = "~/" + name;
		repo.path = path;
		repo.type = type;
		_repos.push_back(repo);
	}
}

void ProjectList::open_project(const repo_t &target)
{
	_open = false;
	_last_project = target.title;
	_host.open_project(target.path);
}
