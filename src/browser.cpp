#include "browser.h"

Browser::Browser():
	_repos(*this)
{
}

void Browser::show_projects()
{
	_repos.show_projects();
	mark_dirty();
}

void Browser::open_project(std::string path)
{
	_project.reset(new DirTree::Root(path));
}

class Button : public ListForm::Field
{
public:
	Button(std::string caption): _caption(caption) {}
	virtual bool active() const override { return true; }
	virtual void paint(WINDOW *view, size_t width) override
	{
		wattron(view, A_BOLD);
		emitch(view, '[', width);
		wattroff(view, A_BOLD);
		emitch(view, ' ', width);
		emitstr(view, _caption, width);
		emitch(view, ' ', width);
		wattron(view, A_BOLD);
		emitch(view, ']', width);
		wattroff(view, A_BOLD);
	}
	virtual void get_highlight(size_t &off, size_t &len) override
	{
		off = 1;
		len = _caption.size() + 2;
	}
private:
	std::string _caption;
};

void Browser::render(ListForm::Builder &lines)
{
	_repos.render(lines);
	lines.blank();
	if (_project) {
		_project->render(lines);
		lines.blank();
	}
	std::unique_ptr<ListForm::Field> field(new Button("Exit"));
	lines.add(std::move(field));
}
