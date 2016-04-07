// ozette
// Copyright (C) 2014-2016 Mars J. Saxman
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

#include "dialog/confirmation.h"
#include "ui/colors.h"
#include <assert.h>

namespace {
class ConfirmationView : public UI::View {
	typedef UI::View inherited;
public:
	ConfirmationView(const Dialog::Confirmation &spec);
	virtual void layout(int vpos, int hpos, int height, int width) override;
	virtual bool process(UI::Frame &ctx, int ch) override;
	virtual void set_help(UI::HelpBar::Panel &panel) override;
protected:
	virtual void paint_into(WINDOW *view, State state) override;
	Dialog::Confirmation _spec;
};
} // namespace anonymous

void Dialog::Confirmation::show(UI::Frame &ctx) {
	std::unique_ptr<UI::View> dptr(new ConfirmationView(*this));
	ctx.show_dialog(std::move(dptr));
}

ConfirmationView::ConfirmationView(const Dialog::Confirmation &spec):
		_spec(spec) {
	assert(!_spec.text.empty());
	assert(_spec.yes != nullptr);
	assert(_spec.no != nullptr);
}

void ConfirmationView::layout(int vpos, int hpos, int height, int width) {
	// A confirmation dialog always has exactly one line, with question text.
	int rows = 1 + _spec.supplement.size();
	inherited::layout(vpos + height - rows, hpos, rows, width);
}

bool ConfirmationView::process(UI::Frame &ctx, int ch) {
	switch (ch) {
		case Control::Escape: ctx.show_result("Cancelled"); return false;
		case 'Y':
		case 'y': _spec.yes(ctx); return false;
		case 'N':
		case 'n': _spec.no(ctx); return false;
	}
	return true;
}

void ConfirmationView::set_help(UI::HelpBar::Panel &panel) {
	panel.yes();
	panel.no();
}

void ConfirmationView::paint_into(WINDOW *view, State state) {
	int height, width;
	getmaxyx(view, height, width);
	(void)height;
	wattrset(view, UI::Colors::dialog(state == State::Focused));
	whline(view, ' ', width);
	int row = 0;
	mvwaddnstr(view, row++, 0, _spec.text.c_str(), width);
	for (std::string line: _spec.supplement) {
		mvwhline(view, row, 0, ' ', width);
		mvwaddnstr(view, row, 2, line.c_str(), width - 2);
		++row;
	}
}

