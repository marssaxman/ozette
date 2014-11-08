#ifndef BROWSER_BROWSER_H
#define BROWSER_BROWSER_H

#include "controller.h"
#include "dirtree.h"
#include "shell.h"

class Browser : public UI::Controller
{
public:
	static void change_directory(std::string path);
	static void open(std::string path, UI::Shell &shell);
	Browser(std::string path);
	virtual void activate(UI::Frame &ctx) override;
	virtual void paint(WINDOW *view, bool active) override;
	virtual bool process(UI::Frame &ctx, int ch) override;
	void view(std::string path);
private:
	~Browser() {_instance = nullptr;}
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
	void key_space(UI::Frame &ctx);
	void set_title(UI::Frame &ctx);
	void build_list();
	void toggle(UI::Frame &ctx);
	void edit_file(UI::Frame &ctx);
	void insert_rows(size_t index, unsigned indent, DirTree *entry);
	void remove_rows(size_t index, unsigned indent);
	DirTree *sel_entry() { return _list[_selection].entry; }
	DirTree _tree;
	std::vector<row_t> _list;
	size_t _selection = 0;
	size_t _scrollpos = 0;
	bool _rebuild_list = true;
};

#endif // BROWSER_BROWSER_H
