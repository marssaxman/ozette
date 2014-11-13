#include "editor.h"
#include "control.h"
#include "dialog.h"
#include <assert.h>
#include <sys/stat.h>
#include <cctype>

Editor::View::View():
	_cursor(_doc, _update)
{
	// new blank buffer
}

Editor::View::View(std::string targetpath):
	_targetpath(targetpath),
	_doc(targetpath),
	_cursor(_doc, _update)
{
}

void Editor::View::activate(UI::Frame &ctx)
{
	// Set the title according to the target path
	if (_targetpath.empty()) {
		ctx.set_title("Untitled");
	} else {
		ctx.set_title(ctx.app().display_path(_targetpath));
	}
	ctx.set_status(_doc.status());
}

void Editor::View::deactivate(UI::Frame &ctx)
{
}

void Editor::View::paint_into(WINDOW *dest, bool active)
{
	update_dimensions(dest);
	if (active != _last_active || dest != _last_dest) {
		_update.all();
		_last_active = active;
		_last_dest = dest;
	}
	for (unsigned i = 0; i < _height; ++i) {
		paint_line(dest, i, active);
	}
	position_t curs = _cursor.position();
	curs.v -= std::min(curs.v, _scrollpos);
	wmove(dest, curs.v, curs.h);
	bool show_cursor = active && _selection.empty();
	curs_set(show_cursor ? 1 : 0);
	_update.reset();
}

bool Editor::View::process(UI::Frame &ctx, int ch)
{
	if (ERR == ch) return true;
	switch (ch) {
		case Control::Cut: ctl_cut(ctx); break;
		case Control::Copy: ctl_copy(ctx); break;
		case Control::Paste: ctl_paste(ctx); break;

		case Control::Close: ctl_close(ctx); break;
		case Control::Save: ctl_save(ctx); break;

		case Control::ToLine: ctl_toline(ctx); break;
		case Control::Find: ctl_find(ctx); break;

		case KEY_DOWN: key_down(false); break;
		case KEY_UP: key_up(false); break;
		case KEY_LEFT: key_left(false); break;
		case KEY_RIGHT: key_right(false); break;
		case KEY_NPAGE: key_page_down(); break;
		case KEY_PPAGE: key_page_up(); break;
		case KEY_HOME: key_home(); break; // move to beginning of line
		case KEY_END: key_end(); break; // move to end of line
		case KEY_SF: key_down(true); break; // shifted down-arrow
		case KEY_SR: key_up(true); break; // shifted up-arrow
		case KEY_SLEFT: key_left(true); break;
		case KEY_SRIGHT: key_right(true); break;

		case Control::Tab: key_tab(ctx); break;
		case Control::Enter: key_enter(ctx); break;
		case Control::Return: key_return(ctx); break;
		case Control::Backspace: key_backspace(ctx); break;
		case KEY_DC: key_delete(ctx); break;
		case KEY_BTAB: break;	// shift-tab
		default: {
			if (isprint(ch)) key_insert(ch);
		} break;
	}
	postprocess(ctx);
	return true;
}

void Editor::View::set_help(UI::HelpBar::Panel &panel)
{
	using namespace UI::HelpBar;
	panel.label[0][0] = Label('X', true, "Cut");
	panel.label[0][1] = Label('C', true, "Copy");
	panel.label[0][2] = Label('V', true, "Paste");
	panel.label[0][4] = Label('L', true, "To Line");
	panel.label[0][5] = Label('F', true, "Find");
	panel.label[1][0] = Label('W', true, "Close");
	panel.label[1][1] = Label('S', true, "Save");
}

void Editor::View::postprocess(UI::Frame &ctx)
{
	reveal_cursor();
	if (_update.has_dirty()) {
		ctx.repaint();
		ctx.set_status(_doc.status());
	}
}

void Editor::View::paint_line(WINDOW *dest, row_t v, bool active)
{
	size_t index = v + _scrollpos;
	if (!_update.is_dirty(index)) return;
	wmove(dest, (int)v, 0);
	auto &line = _doc.line(index);
	line.paint(dest, _width);
	if (!active) return;
	if (_selection.empty()) return;
	column_t selbegin = 0;
	unsigned selcount = 0;
	if (_selection.begin().line < index && _selection.end().line > index) {
		selcount = _width;
	} else if (_selection.begin().line < index && _selection.end().line == index) {
		selcount = line.column(_selection.end().offset);
	} else if (_selection.begin().line == index && _selection.end().line > index) {
		selbegin = line.column(_selection.begin().offset);
		selcount = _width - selbegin;
	} else if (_selection.begin().line == index && _selection.end().line == index) {
		selbegin = line.column(_selection.begin().offset);
		selcount = line.column(_selection.end().offset) - selbegin;
	}
	if (selcount > 0) {
		mvwchgat(dest, v, selbegin, selcount, A_REVERSE, 0, NULL);
	}
}

bool Editor::View::line_is_visible(size_t index) const
{
	return index >= _scrollpos && (index - _scrollpos) < _height;
}

void Editor::View::reveal_cursor()
{
	line_t line = _cursor.location().line;
	// If the cursor is already on screen, do nothing.
	if (line_is_visible(line)) return;
	// Try to center the viewport over the cursor.
	_scrollpos = (line > _halfheight) ? (line - _halfheight) : 0;
	// Don't scroll so far we reveal empty space.
	_scrollpos = std::min(_scrollpos, _maxscroll);
	_update.all();
}

void Editor::View::update_dimensions(WINDOW *view)
{
	int height, width;
	getmaxyx(view, height, width);
	if ((size_t)height != _height) {
		_height = (size_t)height;
		_halfheight = _height / 2;
		_update.all();
	}
	if ((size_t)width != _width) {
		_width = (size_t)width;
		_update.all();
	}
	size_t newmax = std::max(_doc.maxline(), _height) - _halfheight;
	if (newmax != _maxscroll) {
		_maxscroll = newmax;
		_scrollpos = std::min(_scrollpos, _maxscroll);
		_update.all();
	}
}

void Editor::View::ctl_cut(UI::Frame &ctx)
{
	ctl_copy(ctx);
	delete_selection();
}

void Editor::View::ctl_copy(UI::Frame &ctx)
{
	if (_selection.empty()) return;
	std::string clip = _doc.text(_selection);
	ctx.app().set_clipboard(clip);
}

void Editor::View::ctl_paste(UI::Frame &ctx)
{
	delete_selection();
	std::string clip = ctx.app().get_clipboard();
	location_t oldloc = _cursor.location();
	location_t newloc = _doc.insert(oldloc, clip);
	if (oldloc.line != newloc.line) {
		_update.forward(oldloc);
	}
	_cursor.move_to(newloc);
	drop_selection();
}

void Editor::View::ctl_close(UI::Frame &ctx)
{
	if (!_doc.modified()) {
		// no formality needed, we're done
		ctx.app().close_file(_targetpath);
	}
	// ask the user if they want to save first
	std::string prompt = "You have modified this file. Save changes before closing?";
	UI::Dialog::Branch::Option yes;
	yes.key = 'Y';
	yes.description = "Yes";
	yes.action = [this](UI::Frame &ctx)
	{
		// save the file
		_doc.Write(_targetpath);
		ctx.app().close_file(_targetpath);
	};
	UI::Dialog::Branch::Option no;
	no.key = 'N';
	no.description = "No";
	no.action = [this](UI::Frame &ctx)
	{
		// just close it
		ctx.app().close_file(_targetpath);
	};
	std::vector<UI::Dialog::Branch::Option> options = {yes, no};
	auto dialog = new UI::Dialog::Branch(prompt, options);
	std::unique_ptr<UI::View> dptr(dialog);
	ctx.show_dialog(std::move(dptr));
}

void Editor::View::ctl_save(UI::Frame &ctx)
{
	save(ctx, _targetpath);
}

void Editor::View::ctl_toline(UI::Frame &ctx)
{
	UI::Dialog::Input::Layout dialog;
	// illogical as it is, the rest of the world seems to think that it is
	// a good idea for line numbers to start counting at 1, so we will
	// accommodate their perverse desires in the name of compatibility.
	dialog.prompt = "Go to line (";
	dialog.prompt += std::to_string(_cursor.location().line + 1);
	dialog.prompt += ")";
	dialog.commit = [this](UI::Frame &ctx, std::string value)
	{
		if (value.empty()) return;
		long valnum = std::stol(value) - 1;
		size_t index = (valnum >= 0) ? valnum : 0;
		_cursor.move_to(_doc.home(index));
		drop_selection();
		postprocess(ctx);
	};
	std::unique_ptr<UI::View> dptr(new UI::Dialog::Input(dialog));
	ctx.show_dialog(std::move(dptr));
}

void Editor::View::ctl_find(UI::Frame &ctx)
{
	UI::Dialog::Input::Layout dialog;
	dialog.prompt = "Find";
	if (!_find_text.empty()) {
		dialog.prompt += " (" + _find_text + ")";
	}
	dialog.commit = [this](UI::Frame &ctx, std::string value)
	{
		if (!value.empty()) {
			_find_text = value;
		}
		location_t loc = _doc.next(_cursor.location());
		auto next = _doc.find(_find_text, loc);
		if (next == _doc.end()) {
			next = _doc.find(_find_text, _doc.home());
			if (next == _doc.end()) {
				ctx.show_result("Not found");
				next = _cursor.location();
			} else if (next == _cursor.location()) {
				ctx.show_result("This is the only occurrence");
			} else {
				ctx.show_result("Search wrapped");
			}
		}
		_cursor.move_to(next);
		reveal_cursor();
		ctx.repaint();
	};
	std::unique_ptr<UI::View> dptr(new UI::Dialog::Input(dialog));
	ctx.show_dialog(std::move(dptr));
}

void Editor::View::key_up(bool extend)
{
	_cursor.up(1);
	adjust_selection(extend);
}

void Editor::View::key_down(bool extend)
{
	_cursor.down(1);
	adjust_selection(extend);
}

void Editor::View::key_left(bool extend)
{
	_cursor.left();
	adjust_selection(extend);
}

void Editor::View::key_right(bool extend)
{
	_cursor.right();
	adjust_selection(extend);
}

void Editor::View::key_page_up()
{
	_cursor.up(_halfheight+1);
	drop_selection();
}

void Editor::View::key_page_down()
{
	_cursor.down(_halfheight+1);
	drop_selection();
}

void Editor::View::key_home()
{
	_cursor.home();
	drop_selection();
}

void Editor::View::key_end()
{
	_cursor.end();
	drop_selection();
}

void Editor::View::delete_selection()
{
	if (_selection.empty()) return;
	_update.forward(_selection.begin());
	_cursor.move_to(_doc.erase(_selection));
	drop_selection();
}

void Editor::View::key_insert(char ch)
{
	delete_selection();
	_cursor.move_to(_doc.insert(_cursor.location(), ch));
	_anchor = _cursor.location();
	_selection.reset(_anchor);
}

void Editor::View::key_tab(UI::Frame &ctx)
{
	key_insert('\t');
}

void Editor::View::key_enter(UI::Frame &ctx)
{
	// Split the line at the cursor position, but don't move the cursor.
	delete_selection();
	_doc.split(_cursor.location());
	_update.forward(_cursor.location());
}

void Editor::View::key_return(UI::Frame &ctx)
{
	// Split the line at the cursor position and move the cursor to the new line.
	delete_selection();
	_cursor.move_to(_doc.split(_cursor.location()));
	_update.forward(_cursor.location());
}

void Editor::View::key_backspace(UI::Frame &ctx)
{
	if (_selection.empty()) key_left(true);
	delete_selection();
}

void Editor::View::key_delete(UI::Frame &ctx)
{
	if (_selection.empty()) key_right(true);
	delete_selection();
}

void Editor::View::drop_selection()
{
	// The selection is no longer interesting. Move the anchor to the
	// current cursor location and reset the selection around it.
	_anchor = _cursor.location();
	_selection.reset(_anchor);
}

void Editor::View::adjust_selection(bool extend)
{
	if (extend) {
		// The cursor has moved in range-selection mode.
		// Leave the anchor where it is, then extend the
		// selection to include the new cursor point.
		_selection.extend(_anchor, _cursor.location());
	} else {
		// The cursor moved but did not extend the selection.
		drop_selection();
	}
}

void Editor::View::save(UI::Frame &ctx, std::string path)
{
	UI::Dialog::Input::Layout dialog;
	dialog.prompt = "Save File";
	dialog.value = path;
	dialog.commit = [this](UI::Frame &ctx, std::string path)
	{
		// Clearing out the path name is the same as cancelling.
		if (path.empty()) {
			ctx.show_result("Cancelled");
			return;
		}
		// If they confirmed the existing name, we can write it out.
		if (path == _targetpath) {
			_doc.Write(path);
			ctx.set_status(_doc.status());
			std::string stat = "Wrote " + std::to_string(_doc.maxline()+1);
			stat += (_doc.maxline() > 1) ? " lines" : " line";
			ctx.show_result(stat);
			return;
		}
		// This is a different path than the file used to have.
		// Ask the user to confirm that they meant to change it.
		UI::Dialog::Branch::Option yes;
		yes.key = 'Y';
		yes.description = "Yes";
		yes.action = [this, path](UI::Frame &ctx)
		{
			if (path.empty()) return;
			_doc.Write(path);
			_targetpath = path;
			ctx.set_title(path);
		};
		UI::Dialog::Branch::Option no;
		no.key = 'N';
		no.description = "No";
		no.action = [this, path](UI::Frame &ctx)
		{
			save(ctx, path);
		};
		std::string prompt = "Save file under a different name?";
		std::vector<UI::Dialog::Branch::Option> options = {yes, no};
		auto dialog = new UI::Dialog::Branch(prompt, options);
		std::unique_ptr<UI::View> dptr(dialog);
		ctx.show_dialog(std::move(dptr));
	};
	std::unique_ptr<UI::View> dptr(new UI::Dialog::Input(dialog));
	ctx.show_dialog(std::move(dptr));
}

