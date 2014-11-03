#include "browser.h"
#include "repolist.h"
#include "treeview.h"
#include <assert.h>

// In order to avoid deleting the subcontroller out from underneath
// it when it tells us to delegate management to a different controller,
// we will detach it from _sub every time we call it, only reattaching
// if we haven't replaced _sub with some other value in the meantime.

Browser::Browser():
	_sub(new RepoList(*this))
{
}

void Browser::paint(WINDOW *view)
{
	std::unique_ptr<Controller> temp(std::move(_sub));
	temp->paint(view);
	if (!_sub.get()) {
		_sub.swap(temp);
	}
}

bool Browser::process(WINDOW *view, int ch)
{
	std::unique_ptr<Controller> temp(std::move(_sub));
	bool out = temp->process(view, ch);
	if (!_sub.get()) {
		_sub.swap(temp);
	}
	return out;
}

bool Browser::poll(WINDOW *view)
{
	std::unique_ptr<Controller> temp(std::move(_sub));
	bool out = temp->poll(view);
	if (!_sub.get()) {
		_sub.swap(temp);
	}
	return out;
}

std::string Browser::title() const
{
	std::string more = _sub->title();
	if (!more.empty()) {
		more = ": " + more;
	}
	return "Lindi" + more;
}

void Browser::set_project(std::string path)
{
	_sub.reset(new TreeView(*this, path));
}

void Browser::close_project()
{
	_sub.reset(new RepoList(*this));
}
