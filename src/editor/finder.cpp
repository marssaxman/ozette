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

#include "editor/finder.h"
#include "ui/colors.h"

using namespace Editor;

namespace {
class FindView : public UI::View
{
	typedef UI::View inherited;
public:
	// Open a finder and wait for input.
	FindView(Editor::View&, Document&, Range selection);
	// Open a finder and immediately search for the selected text.
	FindView(UI::Frame&, Editor::View&, Document&, Range selection);
	virtual void layout(int vpos, int hpos, int height, int width) override;
	virtual bool process(UI::Frame &ctx, int ch) override;
	virtual void set_help(UI::HelpBar::Panel &panel) override;
protected:
	virtual void paint_into(WINDOW *view, State state) override;
	void input_changed(UI::Frame &ctx);
	void run_find(UI::Frame &ctx);
	void find_next(UI::Frame &ctx);
private:
	Editor::View &_editor;
	Editor::Document &_document;
	Editor::Range _anchor_selection;
	std::unique_ptr<UI::Input> _input;
	std::vector<Range> _matches;
	size_t _found_item;
};
} // namespace

void Editor::Finder::find(
		UI::Frame &ctx, Editor::View &editor, Document &doc, Range selection)
{
	auto view = new FindView(editor, doc, selection);
	std::unique_ptr<UI::View> dptr(view);
	ctx.show_dialog(std::move(dptr));
}

void Editor::Finder::find_next(
		UI::Frame &ctx, Editor::View &editor, Document &doc, Range sel)
{
	auto view = new FindView(ctx, editor, doc, sel);
	std::unique_ptr<UI::View> dptr(view);
	ctx.show_dialog(std::move(dptr));
}

FindView::FindView(Editor::View &editor, Document &doc, Range selection):
	inherited(),
	_editor(editor),
	_document(doc),
	_anchor_selection(selection)
{
	auto updater = [this](UI::Frame &ctx){ input_changed(ctx); };
	_input.reset(new UI::Input("", nullptr, updater));
}

FindView::FindView(
		UI::Frame &ctx, Editor::View &editor, Document &doc, Range selection):
	inherited(),
	_editor(editor),
	_document(doc),
	_anchor_selection(selection)
{
	auto updater = [this](UI::Frame &ctx){ input_changed(ctx); };
	std::string pattern = doc.text(selection);
	_input.reset(new UI::Input(pattern, nullptr, updater));
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
		case Control::FindNext: find_next(ctx); break;
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
		if (_matches.empty()) {
			location += "None found";
		} else {
			location += std::to_string(1 + _found_item);
			location += " of ";
			location += std::to_string(_matches.size());
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
		_matches.clear();
		_found_item = 0;
		_editor.select(ctx, _anchor_selection);
		return;
	}
	location_t anchorpoint = _anchor_selection.begin();
	_matches = _document.find(_input->value());
	_found_item = 0;
	for (auto &match: _matches) {
		if (match.begin() > anchorpoint) break;
		_found_item++;
	}
	if (_found_item >= _matches.size()) {
		_found_item = 0;
	}
	if (!_matches.empty()) {
		_editor.select(ctx, _matches[_found_item]);
	}
}

void FindView::find_next(UI::Frame &ctx)
{
	if (_matches.empty()) return;
	_found_item++;
	if (_found_item >= _matches.size()) {
		_found_item = 0;
	}
	_editor.select(ctx, _matches[_found_item]);
}

