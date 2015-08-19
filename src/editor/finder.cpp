//
// ozette
// Copyright (C) 2015 Mars J. Saxman
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//

#include "ui/view.h"
#include "editor/editor.h"
#include "editor/document.h"
#include "editor/finder.h"
#include "ui/colors.h"
#include "ui/input.h"
#include <assert.h>

using namespace Editor;

namespace {
class FindView : public UI::View
{
	typedef UI::View inherited;
public:
	FindView(UI::Frame& ctx, Finder spec);
	virtual void layout(int vpos, int hpos, int height, int width) override;
	virtual bool process(UI::Frame &ctx, int ch) override;
	virtual void set_help(UI::HelpBar::Panel &panel) override;
protected:
	virtual void paint_into(WINDOW *view, State state) override;
	void input_changed(UI::Frame &ctx);
	void run_find(UI::Frame &ctx);
	void find_next(UI::Frame &ctx);
	void commit(UI::Frame &ctx);
private:
	Finder _finder;
	std::unique_ptr<UI::Input> _input;
	std::unique_ptr<Finder::MatchList> _matches;
	size_t _found_item = 0;
};
} // namespace

void Editor::Finder::show(UI::Frame &ctx)
{
	std::unique_ptr<UI::View> dptr(new FindView(ctx, *this));
	ctx.show_dialog(std::move(dptr));
}

FindView::FindView(UI::Frame &ctx, Finder spec):
	inherited(),
	_finder(spec)
{
	auto updater = [this](UI::Frame &ctx){ input_changed(ctx); };
	_input.reset(new UI::Input(_finder.pattern, nullptr, updater));
	run_find(ctx);
}

void FindView::layout(int vpos, int hpos, int height, int width)
{
	inherited::layout(vpos + height - 1, hpos, 1, width);
}

bool FindView::process(UI::Frame &ctx, int ch)
{
	switch (ch) {
		case Control::Escape: return false;
		case Control::Enter:
		case Control::Return:
		case Control::FindNext: commit(ctx); break;
		default: _input->process(ctx, ch); break;
	}
	return true;
}

void FindView::set_help(UI::HelpBar::Panel &panel)
{
	_input->set_help(panel);
}

void FindView::paint_into(WINDOW *view, State state)
{

	int height, width;
	getmaxyx(view, height, width);
	(void)height;
	wattrset(view, UI::Colors::dialog(state == State::Focused));

	// Draw the caption on the left.
	std::string caption = "Find: ";
	mvwaddnstr(view, 0, 0, caption.c_str(), width);
	int fieldpos = std::min((int)caption.size(), width);
	// If there's still room, draw the location string on the right.
	std::string location;
	if (!_input->value().empty()) {
		location = " (";
		if (_matches) {
			location = _matches->description();
		} else {
			location += "None found";
		}
		location += ")";
	}
	int locpos = std::max(fieldpos, width - (int)location.size());
	mvwaddnstr(view, 0, locpos, location.c_str(), width - locpos);
	// Draw the input field in between caption and location.
	_input->paint(view, 0, fieldpos, locpos - fieldpos, state);
}

void FindView::input_changed(UI::Frame &ctx)
{
	run_find(ctx);
}

void FindView::run_find(UI::Frame &ctx)
{
	// Find all locations in the document which match the current pattern.
	// Identify the location immediately after the current anchor location.
	// Tell the editor to select that location. Update the "X of Y" status.
	std::string pattern = _input->value();
	if (pattern.empty()) {
		_matches.release();
		if (_finder.selector) {
			_finder.selector(ctx, Range(_finder.anchor, _finder.anchor));
		}
		return;
	}
	assert(_finder.matcher);
	_matches = std::move(_finder.matcher(pattern, _finder.anchor));
	if (_matches && _finder.selector) {
		_finder.selector(ctx, _matches->value());
	}
}

void FindView::find_next(UI::Frame &ctx)
{
	if (!_matches) return;
	_matches->next();
	if (_finder.selector) {
		_finder.selector(ctx, _matches->value());
	}
}

void FindView::commit(UI::Frame &ctx)
{
	if (_finder.committer) {
		_finder.committer(_input->value());
	}
	find_next(ctx);
}

