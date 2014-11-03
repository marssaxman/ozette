#ifndef LISTFORM_H
#define LISTFORM_H

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
	virtual bool invoke() { return false; }
	virtual bool cancel() { return false; }
	virtual unsigned indent() const { return 0; }
	virtual void paint(WINDOW *view, size_t width) = 0;
};

class Builder
{
public:
	virtual ~Builder() = default;
	virtual void add(std::unique_ptr<Field> &&field) = 0;
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

class Controller : public ::Controller
{
public:
	virtual void paint(WINDOW *view) override;
	virtual bool process(WINDOW *view, int ch) override;
	virtual bool poll(WINDOW *view) override;
protected:
	virtual void render(Builder &fields) = 0;
private:
	void paint_line(WINDOW *view, int y, int height, int width);
	bool is_selectable(ssize_t line);
	void arrow_down(WINDOW *view);
	void arrow_up(WINDOW *view);
	void commit(WINDOW *view);
	void escape(WINDOW *view);
	void scroll_to_selection(WINDOW *view);
	std::vector<std::unique_ptr<Field>> _lines;
	bool _dirty = true;
	void refresh();
	size_t _scrollpos = 0;
	size_t _selpos = 0;
};

} // namespace ListForm

#endif // LISTFORM_H
