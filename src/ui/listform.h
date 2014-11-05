#ifndef UI_LISTFORM_H
#define UI_LISTFORM_H

#include "app.h"
#include "controller.h"
#include <memory>
#include <vector>
#include <functional>

namespace ListForm {

class Field
{
public:
	virtual ~Field() = default;
	virtual bool active() const { return true; }
	virtual bool invoke(App&) { return invoke(); }
	virtual bool cancel() { return false; }
	virtual void paint(WINDOW *view, size_t width) = 0;
	virtual void get_highlight(size_t &offset, size_t &len) {}
protected:
	virtual bool invoke() { return false; }
	// Common utilities for field implementations
        void emitch(WINDOW *view, int ch, size_t &width);
        void emitstr(WINDOW *view, std::string str, size_t &width);
	void emitrep(WINDOW *view, int ch, size_t repeat, size_t &width);
};

class Builder
{
public:
	virtual ~Builder() = default;
	virtual void add(std::unique_ptr<Field> &&field) = 0;
	virtual void blank() { add(nullptr); }
};

class Source
{
public:
	virtual ~Source() = default;
	virtual void render(Builder &lines) = 0;
};

// A list form is an array of fields, which may have text
// and/or an action. The list controller allows the user to
// scroll between fields with the up and down arrow keys.
// Pressing enter invokes the selected field. Invoking a
// field may cause the list of fields to change.

class Controller : public UI::Controller
{
public:
	virtual void paint(WINDOW *view, bool active) override;
	virtual bool process(Context &ctx, int ch) override;
protected:
	virtual void render(Builder &fields) = 0;
	void mark_dirty() { _dirty = true; }
private:
	void paint_line(WINDOW *view, int y, int height, int width, bool active);
	bool is_selectable(ssize_t line);
	void arrow_down(Context &ctx);
	void arrow_up(Context &ctx);
	void commit(Context &ctx);
	void escape(Context &ctx);
	void scroll_to_selection(WINDOW *view);
	std::vector<std::unique_ptr<Field>> _lines;
	bool _dirty = true;
	void refresh();
	size_t _scrollpos = 0;
	size_t _selpos = 0;
};

} // namespace ListForm

#endif // UI_LISTFORM_H
