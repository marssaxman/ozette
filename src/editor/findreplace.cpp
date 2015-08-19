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
#include "editor/findreplace.h"
#include "ui/colors.h"
#include "ui/input.h"
#include <assert.h>

using namespace Editor;

namespace {
class FindView : public UI::View
{
	typedef UI::View inherited;
public:
	FindView(UI::Frame& ctx, FindReplace spec, bool replace_mode);
	virtual void layout(int vpos, int hpos, int height, int width) override;
	virtual bool process(UI::Frame &ctx, int ch) override;
	virtual void set_help(UI::HelpBar::Panel &panel) override;
protected:
	virtual void paint_into(WINDOW *view, State state) override;
	void key_up(UI::Frame &ctx);
	void key_down(UI::Frame &ctx);
	bool ctl_find(UI::Frame &ctx);
	bool ctl_replace(UI::Frame &ctx);
	void run_find(UI::Frame &ctx);
	void find_next(UI::Frame &ctx);
	void replace_current(UI::Frame &ctx);
	void commit(UI::Frame &ctx);
private:
	FindReplace _spec;
	std::unique_ptr<UI::Input> _pattern; // row 0, always present
	std::unique_ptr<UI::Input> _replacement; // row 1, may not be present
	UI::Input *_active = nullptr;
	std::unique_ptr<FindReplace::MatchList> _matches;
};
} // namespace

void Editor::FindReplace::show(UI::Frame &ctx)
{
	bool replace_mode = false;
	std::unique_ptr<UI::View> dptr(new FindView(ctx, *this, replace_mode));
	ctx.show_dialog(std::move(dptr));
}

FindView::FindView(UI::Frame &ctx, FindReplace spec, bool replace_mode):
	inherited(),
	_spec(spec)
{
	UI::Input *field = nullptr;
	auto update_pattern = [this](UI::Frame &ctx)
	{
		_spec.pattern = _pattern->value();
		run_find(ctx);
	};
	field = new UI::Input(_spec.pattern, nullptr, update_pattern);
	_pattern.reset(field);
	_active = _pattern.get();
	if (replace_mode && _spec.replacer) {
		auto update_replacement = [this](UI::Frame &ctx)
		{
			_spec.replacement = _replacement->value();
		};
		field = new UI::Input(_spec.replacement, nullptr, update_replacement);
		_replacement.reset(field);
	}
	if (!_spec.pattern.empty()) {
		run_find(ctx);
		if (_replacement) {
			_active = _replacement.get();
		}
	}
}

void FindView::layout(int vpos, int hpos, int height, int width)
{
	size_t rows = 1;
	assert(_pattern);
	if (_replacement) rows++;
	inherited::layout(vpos + height - rows, hpos, rows, width);
}

bool FindView::process(UI::Frame &ctx, int ch)
{
	switch (ch) {
		case Control::Escape: return false;
		case KEY_UP: key_up(ctx); break;
		case KEY_DOWN: key_down(ctx); break;
		case Control::Find: return ctl_find(ctx); break;
		case Control::Replace: return ctl_replace(ctx); break;
		case Control::Enter:
		case Control::Return: commit(ctx); break;
		case Control::FindNext: find_next(ctx); break;
		default: _active->process(ctx, ch); break;
	}
	return true;
}

void FindView::set_help(UI::HelpBar::Panel &panel)
{
	_pattern->set_help(panel);
	if (_replacement) {
		panel.find();
	} else if (_spec.replacer) {
		panel.replace();
	}
}

void FindView::paint_into(WINDOW *view, State state)
{

	int height, width;
	getmaxyx(view, height, width);
	(void)height;
	wattrset(view, UI::Colors::dialog(state == State::Focused));
	State normal_state = state;
	if (normal_state == State::Focused) {
		normal_state = State::Active;
	}
	// Draw the Find field.
	// Draw the caption on the left.
	std::string caption = "Find: ";
	// Captions must have equal lengths, so they line up correctly. If there
	// is a replacement field, add spaces to the caption appropriately.
	if (_replacement) caption = "   " + caption;
	mvwaddnstr(view, 0, 0, caption.c_str(), width);
	int fieldpos = std::min((int)caption.size(), width);
	// If there's still room, draw the location string on the right.
	std::string location;
	if (!_pattern->value().empty()) {
		location = " (";
		if (_matches) {
			location += _matches->description();
		} else {
			location += "None found";
		}
		location += ")";
	}
	int locpos = std::max(fieldpos, width - (int)location.size());
	mvwaddnstr(view, 0, locpos, location.c_str(), width - locpos);
	// Draw the input field in between caption and location.
	int fieldwidth = locpos - fieldpos;
	State fieldstate = (_active == _pattern.get())? state: normal_state;
	_pattern->paint(view, 0, fieldpos, fieldwidth, fieldstate);

	if (!_replacement) return;
	// Draw the Replace field. Again, draw its caption on the left.
	caption = "Replace: ";
	mvwaddnstr(view, 1, 0, caption.c_str(), width);
	fieldpos = std::min((int)caption.size(), width);
	fieldwidth = width - fieldpos;
	fieldstate = (_active == _replacement.get())? state: normal_state;
	_replacement->paint(view, 1, fieldpos, fieldwidth, fieldstate);
}

void FindView::key_up(UI::Frame &ctx)
{
	if (_active != _pattern.get()) {
		_active = _pattern.get();
		ctx.repaint();
	}
}

void FindView::key_down(UI::Frame &ctx)
{
	if (_replacement && _active != _replacement.get()) {
		_active = _replacement.get();
		ctx.repaint();
	}
}

bool FindView::ctl_find(UI::Frame &ctx)
{
	// If we are already in find mode, there is nothing to do.
	if (!_replacement) return true;
	// Otherwise, reopen this spec in find-only mode, hiding the replacement
	// field by closing the current dialog.
	std::unique_ptr<UI::View> dptr(new FindView(ctx, _spec, false));
	ctx.show_dialog(std::move(dptr));
	return false;
}

bool FindView::ctl_replace(UI::Frame &ctx)
{
	// If we are already in replace mode, there is nothing to do.
	if (_replacement) return true;
	// If there is no replace function to call, there is no reason to show
	// the replace field.
	if (!_spec.replacer) return true;
	// Otherwise, reopen this dialog spec, showing the replace field.
	std::unique_ptr<UI::View> dptr(new FindView(ctx, _spec, true));
	ctx.show_dialog(std::move(dptr));
	return false;
}

void FindView::run_find(UI::Frame &ctx)
{
	// Find all locations in the document which match the current pattern.
	// Identify the location immediately after the current anchor location.
	// Tell the editor to select that location. Update the "X of Y" status.
	if (_spec.pattern.empty()) {
		_matches.release();
	} else if (_spec.matcher) {
		_matches = std::move(_spec.matcher(_spec.pattern, _spec.anchor));
	}
	if (_spec.selector) {
		_spec.selector(ctx, _matches? _matches->value(): _spec.anchor);
	}
}

void FindView::find_next(UI::Frame &ctx)
{
	if (!_matches) return;
	_matches->next();
	if (_spec.selector) {
		_spec.selector(ctx, _matches->value());
	}
}

void FindView::replace_current(UI::Frame &ctx)
{
	if (!_matches) return;
	if (!_spec.replacer) return;
	_spec.anchor = _spec.replacer(ctx, _matches->value(), _spec.replacement);
	run_find(ctx);
}

void FindView::commit(UI::Frame &ctx)
{
	if (_spec.pattern.empty()) return;
	if (_spec.commit_find) {
		_spec.commit_find(_spec.pattern);
	}
	_pattern->select_all(ctx);
	if (!_matches) {
		_active = _pattern.get();
		ctx.repaint();
		return;
	}
	_spec.anchor = _matches->value();
	if (_replacement) {
		_replacement->select_all(ctx);
		replace_current(ctx);
	} else {
		find_next(ctx);
	}
}

