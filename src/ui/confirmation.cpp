//
// ozette
// Copyright (C) 2014-2015 Mars J. Saxman
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

#include "ui/confirmation.h"
#include "ui/colors.h"
#include <assert.h>

namespace {
class ConfirmationView : public UI::View {
	typedef UI::View inherited;
public:
	typedef std::function<void(UI::Frame&)> action_t;
	ConfirmationView(const UI::Confirmation &confirmation);

	virtual void layout(int vpos, int hpos, int height, int width) override;
	virtual bool process(UI::Frame &ctx, int ch) override;
	virtual void set_help(UI::HelpBar::Panel &panel) override;
protected:
	virtual void paint_into(WINDOW *view, State state) override;
	std::string _text;
	action_t _yes = nullptr;
	action_t _no = nullptr;
};
} // namespace anonymous

void UI::Confirmation::show(UI::Frame &ctx)
{
	std::unique_ptr<UI::View> dptr(new ConfirmationView(*this));
	ctx.show_dialog(std::move(dptr));
}

ConfirmationView::ConfirmationView(const UI::Confirmation &confirmation):
	_text(confirmation.text),
	_yes(confirmation.yes),
	_no(confirmation.no)
{
	assert(!_text.empty());
	assert(_yes != nullptr);
	assert(_no != nullptr);
}

void ConfirmationView::layout(int vpos, int hpos, int height, int width)
{
	// A confirmation dialog always has exactly one line, with question text.
	inherited::layout(vpos + height - 1, hpos, 1, width);
}

bool ConfirmationView::process(UI::Frame &ctx, int ch)
{
	switch (ch) {
		case Control::Escape: ctx.show_result("Cancelled"); return false;
		case 'Y':
		case 'y': _yes(ctx); return false;
		case 'N':
		case 'n': _no(ctx); return false;
	}
	return true;
}

void ConfirmationView::set_help(UI::HelpBar::Panel &panel)
{
	panel.yes();
	panel.no();
}

void ConfirmationView::paint_into(WINDOW *view, State state)
{
	int height, width;
	getmaxyx(view, height, width);
	(void)height;
	wattrset(view, UI::Colors::dialog(state == State::Focused));
	whline(view, ' ', width);
	mvwaddnstr(view, 0, 0, _text.c_str(), width);
}

