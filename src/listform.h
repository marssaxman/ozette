#ifndef LISTFORM_H
#define LISTFORM_H

#include "controller.h"
#include <memory>
#include <vector>

// ListForm implements the UI for the browser controllers.
// It expects the view to contain an array of lines, which
// may be selectable or nonselectable, and which may either
// invoke some action or which may open a text field, which
// may invoke an action when the enter key is pressed.


class ListForm : public Controller
{
public:
	virtual void paint(WINDOW *view) override;
	virtual bool process(WINDOW *view, int ch) override;
	virtual bool poll(WINDOW *view) override;
protected:
	// A form is a scrollable area beginning with some
	// number of command entries and following with a tree
	// of file or directory entries.
	// You can navigate up and down inside this form using
	// the arrow keys.
	// Pressing enter on a command entry opens a text entry
	// area at the bottom of the window. Pressing escape
	// closes the text area back up again; pressing enter
	// invokes the command with that text as parameter.
	// Pressing enter on a file or directory entry invokes
	// some action depending on the specific form.
	class Field
	{
	public:
		virtual ~Field() = default;
		virtual std::string text() const = 0;
		virtual void invoke() = 0;
	};
	std::vector<std::unique_ptr<Field>> _commands;
	std::string _entry_label;
	std::vector<std::unique_ptr<Field>> _entries;
private:
	void paint_line(WINDOW *view, int y, int height, int width);
	bool is_selectable(ssize_t line);
	void arrow_down(WINDOW *view);
	void arrow_up(WINDOW *view);
	void commit(WINDOW *view);
	void escape(WINDOW *view);
	struct Line
	{
		Line(): field(nullptr), text("") {}
		Line(std::string t): field(nullptr), text(t) {}
		Line(Field *f): field(f), text(f->text()) {}
		Field *field;
		std::string text;
	};
	std::vector<Line> _lines;
	bool _dirty = true;
	void refresh();
	size_t _scrollpos = 0;
	size_t _selpos = 0;
};
#endif // LISTFORM_H
