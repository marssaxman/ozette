#ifndef LISTFORM_H
#define LISTFORM_H

#include "controller.h"
#include <memory>
#include <vector>
#include <functional>

// A list form is an array of fields, which may have text
// and/or an action. The list controller allows the user to
// scroll between fields with the up and down arrow keys.
// Pressing enter invokes the selected field. Invoking a
// field may cause the list of fields to change.

class ListForm : public Controller
{
public:
	virtual void paint(WINDOW *view) override;
	virtual bool process(WINDOW *view, int ch) override;
	virtual bool poll(WINDOW *view) override;
	class Builder
	{
	public:
		virtual ~Builder() = default;
		void blank() { entry("", nullptr); }
		void label(std::string text) { entry(text, nullptr); }
		virtual void entry(std::string text, std::function<void()> action) = 0;
	};
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
	struct Line
	{
		std::function<void()> action = nullptr;
		std::string left_text;
		std::string right_text;
	};
	std::vector<Line> _lines;
	class LineBuilder : public Builder
	{
		friend class ListForm;
		LineBuilder(std::vector<Line> &lines): _lines(lines) {}
		std::vector<Line> &_lines;
	public:
		virtual void entry(std::string text, std::function<void()> action) override;
	};
	bool _dirty = true;
	void refresh();
	size_t _scrollpos = 0;
	size_t _selpos = 0;
};
#endif // LISTFORM_H
