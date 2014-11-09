#include "browser.h"
#include "control.h"
#include "dialog.h"

Browser *Browser::_instance;
static std::string kExpansionStateKey = "expanded_dirs";

void Browser::change_directory(std::string path)
{
	if (_instance) _instance->view(path);
}

void Browser::open(std::string path, UI::Shell &shell)
{
	if (_instance) {
		shell.make_active(_instance->_window);
	} else {
		std::unique_ptr<UI::View> view(new Browser(path));
		_instance->_window = shell.open_window(std::move(view));
	}
}

Browser::Browser(std::string path):
	_tree(path)
{
	_instance = this;
}

void Browser::activate(UI::Frame &ctx)
{
	ctx.set_title(_tree.path());
	using namespace Control;
	Panel help = {{
		{Open, NewFile, 0, 0, 0, 0},
		{Quit, 0, 0, Directory, 0, 0},
	}};
	ctx.set_help(help);

	if (_expanded_items.empty()) {
		std::vector<std::string> paths;
		ctx.app().get_config(kExpansionStateKey, paths);
		for (auto &path: paths) {
			_expanded_items.insert(path);
		}
		if (!paths.empty()) _rebuild_list = true;
	}

	if (_rebuild_list) ctx.repaint();
}

void Browser::deactivate(UI::Frame &ctx)
{
	std::vector<std::string> paths;
	for (auto &line: _expanded_items) {
		paths.push_back(line);
	}
	ctx.app().set_config(kExpansionStateKey, paths);
}

void Browser::paint(WINDOW *view, bool active)
{
	int height, width;
	getmaxyx(view, height, width);

	// adjust scrolling as necessary to keep the cursor visible
	size_t max_visible_row = _scrollpos + height - 2;
	if (_selection < _scrollpos || _selection > max_visible_row) {
		size_t halfpage = (size_t)height/2;
		_scrollpos = _selection - std::min(halfpage, _selection);
	}

	int row = 1;
	wmove(view, 0, 0);
	wclrtoeol(view);
	for (size_t i = _scrollpos; i < _list.size() && row < height; ++i) {
		wmove(view, row, 0);
		whline(view, ' ', width);
		paint_row(view, row, _list[i], width);
		if (active && i == _selection) {
			mvwchgat(view, row, 0, width, A_REVERSE, 0, NULL);
		}
		row++;
	}
	while (row < height) {
		wmove(view, row++, 0);
		wclrtoeol(view);
	}
}

bool Browser::process(UI::Frame &ctx, int ch)
{
	if (_rebuild_list) {
		build_list();
		ctx.set_title(_tree.path());
		_rebuild_list = false;
		ctx.repaint();
	}
	switch (ch) {
		case Control::Return: key_return(ctx); break;
		case Control::Close: return false; break;
		case KEY_UP: key_up(ctx); break;
		case KEY_DOWN: key_down(ctx); break;
		case ' ': key_space(ctx); break;
		default: break;
	}
	return true;
}

void Browser::view(std::string path)
{
	if (path != _tree.path()) {
		_list.clear();
		_tree = DirTree(path);
		_rebuild_list = true;
	}
}

void Browser::paint_row(WINDOW *view, int vpos, row_t &display, int width)
{
	int rowchars = width;
	for (unsigned tab = 0; tab < display.indent; tab++) {
		waddnstr(view, "    ", rowchars);
		rowchars -= 4;
	}
	bool isdir = display.entry->is_directory();
	waddnstr(view, display.expanded? "- ": (isdir? "+ ": "  "), rowchars);
	rowchars -= 2;
	std::string name = display.entry->name();
	waddnstr(view, name.c_str(), rowchars - 1);
	rowchars -= std::min(rowchars, (int)name.size());
	waddnstr(view, isdir? "/": " ", rowchars);
	rowchars--;
	if (display.entry->is_file()) {
		char buf[256];
		time_t mtime = display.entry->mtime();
		// add an extra space on the end because it's prettier
		size_t dchars = strftime(buf, 255, "%c ", localtime(&mtime));
		int drawch = std::min((int)dchars, rowchars);
		mvwaddnstr(view, vpos, width-drawch, buf, drawch);
	}
}

void Browser::key_return(UI::Frame &ctx)
{
	switch (sel_entry()->type()) {
		case DirTree::Type::Directory: toggle(ctx); break;
		case DirTree::Type::File: edit_file(ctx); break;
		default: break;
	}
}

void Browser::key_up(UI::Frame &ctx)
{
	if (0 == _selection) return;
	_selection--;
	ctx.repaint();
}

void Browser::key_down(UI::Frame &ctx)
{
	if (_selection + 1 == _list.size()) return;
	_selection++;
	ctx.repaint();
}

void Browser::key_space(UI::Frame &ctx)
{
	toggle(ctx);
}

void Browser::build_list()
{
	_list.clear();
	insert_rows(0, 0, &_tree);
	if (_list.empty()) {
		row_t display = {0, 0, &_tree};
		_list.insert(_list.begin(), display);
	}
	_selection = std::min(_selection, _list.size());
}

void Browser::toggle(UI::Frame &ctx)
{
	auto &display = _list[_selection];
	auto entry = display.entry;
	if (!entry->is_directory()) return;
	if (display.expanded) {
		// collapse it
		_expanded_items.erase(display.entry->path());
		display.expanded = false;
		remove_rows(_selection + 1, display.indent + 1);
	} else {
		// expand it
		_expanded_items.insert(display.entry->path());
		display.expanded = true;
		insert_rows(_selection + 1, display.indent + 1, entry);
	}
	ctx.repaint();
}

void Browser::edit_file(UI::Frame &ctx)
{
	auto &display = _list[_selection];
	ctx.app().edit_file(display.entry->path());
}

size_t Browser::insert_rows(size_t index, unsigned indent, DirTree *entry)
{
	for (auto &item: entry->items()) {
		bool expand = _expanded_items.count(item.path()) > 0;
		row_t display = {indent, expand, &item};
		_list.insert(_list.begin() + index++, display);
		if (expand) {
			index = insert_rows(index, indent + 1, &item);
		}
	}
	return index;
}

void Browser::remove_rows(size_t index, unsigned indent)
{
	auto begin = _list.begin() + index;
	auto iter = begin;
	while (iter != _list.end() && iter->indent >= indent) {
		iter++;
	}
	_list.erase(begin, iter);
}
