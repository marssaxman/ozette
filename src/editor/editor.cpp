//
// lindi
// Copyright (C) 2014 Mars J. Saxman
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

#include "editor.h"
#include "control.h"
#include "dialog.h"
#include <assert.h>
#include <sys/stat.h>
#include <dirent.h>
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

Editor::View::View(std::string title, Document &&doc):
	_targetpath(title),
	_doc(std::move(doc)),
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
	set_status(ctx);
}

void Editor::View::deactivate(UI::Frame &ctx)
{
	_doc.commit();
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
	curs.h -= std::min(curs.h, _scroll.h);
	curs.v -= std::min(curs.v, _scroll.v);
	wmove(dest, curs.v, curs.h);
	bool show_cursor = active && _selection.empty();
	show_cursor &= !_doc.readonly();
	curs_set(show_cursor ? 1 : 0);
	_update.reset();
}

void Editor::View::clear_overlay()
{
	_update.all();
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
		case Control::Undo: ctl_undo(ctx); break;
		case Control::Redo: ctl_redo(ctx); break;
		case Control::DownArrow: ctl_open_next(ctx); break;

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
		case KEY_BTAB: key_btab(ctx); break;	// shift-tab
		default: {
			if (isprint(ch)) key_insert(ch);
			else ctx.show_result("Unknown control: " + std::to_string(ch));
		} break;
	}
	postprocess(ctx);
	return true;
}

void Editor::View::set_help(UI::HelpBar::Panel &panel)
{
	using namespace UI::HelpBar;
	if (!_doc.readonly()) {
		panel.label[0][0] = Label('X', true, "Cut");
		panel.label[0][1] = Label('C', true, "Copy");
		panel.label[0][2] = Label('V', true, "Paste");
	}
	panel.label[0][4] = Label('L', true, "To Line");
	panel.label[0][5] = Label('F', true, "Find");
	panel.label[1][0] = Label('W', true, "Close");
	if (!_doc.readonly()) {
		panel.label[1][1] = Label('S', true, "Save");
	}
	if (_doc.can_redo()) {
		panel.label[1][3] = Label('Y', true, "Redo");
	}
	if (_doc.can_undo()) {
		panel.label[1][4] = Label('Z', true, "Undo");
	}
	panel.label[1][5] = Label('?', true, "Help");
}

void Editor::View::postprocess(UI::Frame &ctx)
{
	reveal_cursor();
	if (_update.has_dirty()) {
		ctx.repaint();
		set_status(ctx);
	}
}

void Editor::View::paint_line(WINDOW *dest, row_t v, bool active)
{
	size_t index = v + _scroll.v;
	if (!_update.is_dirty(index)) return;
	wmove(dest, (int)v, 0);
	DisplayLine line = _doc.display(index);
	line.paint(dest, _scroll.h, _width);
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

void Editor::View::reveal_cursor()
{
	if (_doc.readonly()) return;
	// If the cursor is on a line which is not on screen, scroll vertically to
	// position the line in the center of the window.
	line_t line = _cursor.location().line;
	if (line < _scroll.v || (line - _scroll.v) >= _height) {
		// Try to center the viewport over the cursor.
		_scroll.v = (line > _halfheight) ? (line - _halfheight) : 0;
		// Don't scroll so far we reveal empty space.
		_scroll.v = std::min(_scroll.v, _maxscroll);
		_update.all();
	}
	// Try to keep the view scrolled left if possible, but if that would put the
	// cursor offscreen, scroll right by the cursor position plus one tab width.
	if (_cursor.position().h >= _width) {
		column_t newh = _cursor.position().h + Editor::kTabWidth - _width;
		if (newh != _scroll.h) {
			_scroll.h = newh;
			_update.all();
		}
	} else if (_scroll.h > 0) {
		_scroll.h = 0;
		_update.all();
	}
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
	row_t newmax = std::max((row_t)_doc.maxline(), _height) - _halfheight;
	if (newmax != _maxscroll) {
		_maxscroll = newmax;
		_scroll.v = std::min(_scroll.v, _maxscroll);
		_update.all();
	}
}

void Editor::View::set_status(UI::Frame &ctx)
{
	std::string status = _doc.status();
	if (!_doc.readonly()) {
		if (!status.empty()) status.push_back(' ');
		status.push_back('@');
		// humans use weird 1-based line numbers
		status += std::to_string(1 + _cursor.location().line);
	}
	ctx.set_status(status);
}

void Editor::View::ctl_cut(UI::Frame &ctx)
{
	ctl_copy(ctx);
	delete_selection();
	_doc.commit();
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
	_doc.commit();
}

void Editor::View::ctl_close(UI::Frame &ctx)
{
	if (_doc.readonly() || !_doc.modified()) {
		// no formality needed, we're done
		ctx.app().close_file(_targetpath);
		return;
	}
	// ask the user if they want to save first
	std::string prompt = "You have modified this file. Save changes before closing?";
	auto yes_action = [this](UI::Frame &ctx)
	{
		// save the file
		_doc.Write(_targetpath);
		ctx.app().close_file(_targetpath);
	};
	auto no_action = [this](UI::Frame &ctx)
	{
		// just close it
		ctx.app().close_file(_targetpath);
	};
	auto dialog = new UI::Dialog::Confirmation(prompt, yes_action, no_action);
	std::unique_ptr<UI::View> dptr(dialog);
	ctx.show_dialog(std::move(dptr));
}

void Editor::View::ctl_save(UI::Frame &ctx)
{
	save(ctx, _targetpath);
	_doc.commit();
}

void Editor::View::ctl_toline(UI::Frame &ctx)
{
	// illogical as it is, the rest of the world seems to think that it is
	// a good idea for line numbers to start counting at 1, so we will
	// accommodate their perverse desires in the name of compatibility.
	std::string prompt = "Go to line (";
	prompt += std::to_string(_cursor.location().line + 1);
	prompt += ")";
	auto commit = [this](UI::Frame &ctx, std::string value)
	{
		if (value.empty()) return;
		long valnum = std::stol(value) - 1;
		size_t index = (valnum >= 0) ? valnum : 0;
		_cursor.move_to(_doc.home(index));
		drop_selection();
		postprocess(ctx);
	};
	auto dialog = new UI::Dialog::GoLine(prompt, commit);
	std::unique_ptr<UI::View> dptr(dialog);
	ctx.show_dialog(std::move(dptr));
}

void Editor::View::ctl_find(UI::Frame &ctx)
{
	std::string prompt = "Find";
	if (!_find_text.empty()) {
		prompt += " (" + _find_text + ")";
	}
	auto commit = [this](UI::Frame &ctx, std::string value)
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
		postprocess(ctx);
	};
	auto dialog = new UI::Dialog::Find(prompt, commit);
	std::unique_ptr<UI::View> dptr(dialog);
	ctx.show_dialog(std::move(dptr));
}

void Editor::View::ctl_undo(UI::Frame &ctx)
{
	_cursor.move_to(_doc.undo(_update));
	_anchor = _cursor.location();
	_selection.reset(_anchor);
	postprocess(ctx);
}

void Editor::View::ctl_redo(UI::Frame &ctx)
{
	_cursor.move_to(_doc.redo(_update));
	_anchor = _cursor.location();
	_selection.reset(_anchor);
	postprocess(ctx);
}

void Editor::View::ctl_open_next(UI::Frame &ctx)
{
	// If there is more than one file with the same base name as this one,
	// open the next one, wrapping around after the last. For example, "open
	// next" when editing "foo.c" would open "foo.h", and vice versa.
	std::string dirpath;
	std::string filename;
	size_t slashpos = _targetpath.find_last_of('/');
	if (slashpos != std::string::npos) {
		// Split the file name off from the directory path.
		dirpath = _targetpath.substr(0, slashpos);
		filename = _targetpath.substr(slashpos + 1);
	} else {
		// If the path is a bare file name, assume the current working dir.
		dirpath = ".";
		filename = _targetpath;
	}
	if (filename.empty()) return;
	// Chop off the file name's extension, but keep the dot so we can tell that
	// "footy.c" should not come after "foo.h".
	size_t dotpos = filename.find_last_of('.');
	std::string basename = filename.substr(0, dotpos + 1);
	// Enumerate the files in the target directory, matching each one against
	// the base name, looking for the next one after the one we're editing.
	DIR *pdir = opendir(dirpath.c_str());
	if (!pdir) return;
	std::string match;
	bool found_current = false;
	while (dirent *entry = readdir(pdir)) {
		if (entry->d_name[0] == '.') continue;
		std::string entry_name = std::string(entry->d_name);
		if (entry_name.substr(0, basename.size()) != basename) continue;
		if (match.empty() || found_current) match = entry_name;
		if (found_current) break;
		found_current = entry_name == filename;
	}
	closedir(pdir);
	if (match.empty()) return;
	ctx.app().edit_file(dirpath + "/" + match);
}

void Editor::View::key_up(bool extend)
{
	if (_doc.readonly()) {
		_scroll.v -= std::min(_scroll.v, 1U);
		_update.all();
	} else {
		_cursor.up();
		adjust_selection(extend);
	}
}

void Editor::View::key_down(bool extend)
{
	if (_doc.readonly()) {
		_scroll.v = std::min(_scroll.v + 1, _maxscroll);
		_update.all();
	} else {
		_cursor.down();
		adjust_selection(extend);
	}
}

void Editor::View::key_left(bool extend)
{
	if (_doc.readonly()) return;
	_cursor.left();
	adjust_selection(extend);
}

void Editor::View::key_right(bool extend)
{
	if (_doc.readonly()) return;
	_cursor.right();
	adjust_selection(extend);
}

void Editor::View::key_page_up()
{
	// move the cursor to the last line of the previous page
	_cursor.move_to(_doc.home(_scroll.v - std::min(_scroll.v, 1U)));
	drop_selection();
}

void Editor::View::key_page_down()
{
	// move the cursor to the first line of the next page
	_cursor.move_to(_doc.home(_scroll.v + _height));
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
	if (_selection.empty()) {
		key_insert('\t');
	} else {
		// indent all lines touched by the selection one more tab then extend
		// the selection to encompass all of those lines, because that's what
		// VS does and I like it that way.
		line_t begin = _selection.begin().line;
		line_t end = _selection.end().line;
		if (end > begin && 0 == _selection.end().offset) end--;
		for (line_t index = begin; index <= end; ++index) {
			_doc.insert(_doc.home(index), '\t');
		}
		_anchor = _doc.home(begin);
		_cursor.move_to(_doc.end(end));
		_selection.reset(_anchor, _cursor.location());
		_update.range(_selection);
	}
}

void Editor::View::key_btab(UI::Frame &ctx)
{
	// Shift-tab unindents the selection, if present, or simply the current
	// line if there is no selection.
	// Remove the leftmost tab character from all of the selected lines, then
	// extend the selection to encompass all of those lines.
	line_t begin = _selection.begin().line;
	line_t end = _selection.end().line;
	if (end > begin && 0 == _selection.end().offset) end--;
	for (line_t index = begin; index <= end; ++index) {
		std::string text = _doc.line(index);
		if (text.empty()) continue;
		if (text.front() != '\t') continue;
		location_t pretab = _doc.home(index);
		location_t posttab = _doc.next(pretab);
		_doc.erase(Range(pretab, posttab));
	}
	_anchor = _doc.home(begin);
	_cursor.move_to(_doc.end(end));
	_selection.reset(_anchor, _cursor.location());
	_update.range(_selection);
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
	line_t old_index = _cursor.location().line;
	_cursor.move_to(_doc.split(_cursor.location()));
	// Add whatever string of whitespace characters begins the previous line.
	for (char ch: _doc.line(old_index)) {
		if (!isspace(ch)) break;
		key_insert(ch);
	}
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
	_update.range(_selection);
	_anchor = _cursor.location();
	_selection.reset(_anchor);
}

void Editor::View::adjust_selection(bool extend)
{
	if (extend) {
		// The cursor has moved in range-selection mode.
		// Leave the anchor where it is, then extend the
		// selection to include the new cursor point.
		_selection.reset(_anchor, _cursor.location());
	} else {
		// The cursor moved but did not extend the selection.
		drop_selection();
	}
}

void Editor::View::save(UI::Frame &ctx, std::string path)
{
	if (_doc.readonly()) return;
	std::string prompt = "Save File";
	auto commit = [this](UI::Frame &ctx, std::string path)
	{
		// Clearing out the path name is the same as cancelling.
		if (path.empty()) {
			ctx.show_result("Cancelled");
			return;
		}
		auto do_yes = [this, path](UI::Frame &ctx)
		{
			if (path.empty()) return;
			_doc.Write(path);
			if (_targetpath != path) {
				ctx.app().rename_file(_targetpath, path);
				_targetpath = path;
				ctx.set_title(path);
			}
			ctx.set_status(_doc.status());
			std::string stat = "Wrote " + std::to_string(_doc.maxline()+1);
			stat += (_doc.maxline() > 1) ? " lines" : " line";
			ctx.show_result(stat);
		};
		auto do_no = [this, path](UI::Frame &ctx)
		{
			save(ctx, path);
		};
		// If the file name entered at the prompt is the same as the existing
		// name, just save the file. Otherwise, ask the user to confirm that
		// they meant to change the path string.
		if (path == _targetpath || _targetpath.empty()) {
			do_yes(ctx);
			return;
		}
		std::string prompt = "Save file under a different name?";
		auto dialog = new UI::Dialog::Confirmation(prompt, do_yes, do_no);
		std::unique_ptr<UI::View> dptr(dialog);
		ctx.show_dialog(std::move(dptr));
	};
	auto dialog = new UI::Dialog::Save(prompt, path, commit);
	std::unique_ptr<UI::View> dptr(dialog);
	ctx.show_dialog(std::move(dptr));
}

