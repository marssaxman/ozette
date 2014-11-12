#ifndef BROWSER_BROWSER_H
#define BROWSER_BROWSER_H

#include "view.h"
#include "dirtree.h"
#include "shell.h"
#include <set>

class Browser : public UI::View
{
public:
	static void change_directory(std::string path);
	static void open(std::string path, UI::Shell &shell);
	Browser(std::string path);
	virtual void activate(UI::Frame &ctx) override;
	virtual void deactivate(UI::Frame &ctx) override;
	virtual bool process(UI::Frame &ctx, int ch) override;
	virtual void set_help(UI::HelpBar::Panel &panel) override;
	void view(std::string path);
protected:
	virtual void paint_into(WINDOW *view, bool active) override;
private:
	~Browser() { _instance = nullptr; }
	static Browser *_instance;
	UI::Window *_window = nullptr;
	struct row_t {
		unsigned indent;
		bool expanded;
		DirTree *entry;
	};
	void paint_row(WINDOW *view, int vpos, row_t &display, int width);
	void key_return(UI::Frame &ctx);
	void key_up(UI::Frame &ctx);
	void key_down(UI::Frame &ctx);
	void key_left(UI::Frame &ctx);
	void key_right(UI::Frame &ctx);
	void key_space(UI::Frame &ctx);
	void key_char(UI::Frame &ctx, char ch);
	void clear_filter(UI::Frame &ctx);
	bool matches_filter(size_t index);
	void set_title(UI::Frame &ctx);
	void build_list();
	void toggle(UI::Frame &ctx);
	void edit_file(UI::Frame &ctx);
	size_t insert_rows(size_t index, unsigned indent, DirTree *entry);
	void remove_rows(size_t index, unsigned indent);
	DirTree *sel_entry() { return _list[_selection].entry; }
	DirTree _tree;
	std::vector<row_t> _list;
	std::set<std::string> _expanded_items;
	size_t _selection = 0;
	size_t _scrollpos = 0;
	bool _rebuild_list = true;
	std::string _name_filter;
};

#endif // BROWSER_BROWSER_H
