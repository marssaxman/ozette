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

#include <assert.h>
#include <cctype>
#include <dirent.h>
#include <sys/stat.h>
#include "app/control.h"
#include "app/path.h"
#include "dialog/confirmation.h"
#include "dialog/form.h"
#include "editor/displayline.h"
#include "editor/editor.h"
#include "ui/colors.h"
#include "search/dialog.h"

Editor::View::View():
		_syntax(Syntax::lookup("")) {
	// new blank buffer
}

Editor::View::View(std::string targetpath):
		_targetpath(targetpath),
		_doc(targetpath),
		_syntax(Syntax::lookup(targetpath)) {
	_config.load(targetpath);
}

Editor::View::View(std::string title, Document &&doc):
	_targetpath(title),
	_doc(std::move(doc)),
	_syntax(Syntax::lookup(title)) {
}

void Editor::View::activate(UI::Frame &ctx) {
	// Set the title according to the target path
	if (_targetpath.empty()) {
		ctx.set_title("Untitled");
	} else {
		ctx.set_title(Path::display(_targetpath));
	}
	set_status(ctx);
}

void Editor::View::deactivate(UI::Frame &ctx) {
	_doc.commit();
}

void Editor::View::paint_into(WINDOW *dest, State state) {
	update_dimensions(dest);
	if (state != _last_state || dest != _last_dest) {
		_update.all();
		_last_state = state;
		_last_dest = dest;
	}
	for (unsigned i = 0; i < _height; ++i) {
		paint_line(dest, i, state);
	}
	position_t curs = to_position(_cursor);
	curs.h -= std::min(curs.h, _scroll.h);
	curs.v -= std::min(curs.v, _scroll.v);
	wmove(dest, curs.v, curs.h);
	bool show_cursor = (state == State::Focused) && _selection.empty();
	show_cursor &= !_doc.readonly();
	curs_set(show_cursor ? 1 : 0);
	_update.reset();
}

void Editor::View::clear_overlay() {
	_update.all();
}

bool Editor::View::process(UI::Frame &ctx, int ch) {
	if (ERR == ch) return true;
	switch (ch) {
		case Control::Cut: ctl_cut(ctx); break;
		case Control::Copy: ctl_copy(ctx); break;
		case Control::Paste: ctl_paste(ctx); break;
		case Control::Close: ctl_close(ctx); break;
		case Control::Save: ctl_save(ctx); break;
		case Control::SaveAs: ctl_save_as(ctx); break;
		case Control::ToLine: ctl_toline(ctx); break;
		case Control::Find: ctl_find(ctx); break;
		case Control::Replace: ctl_replace(ctx); break;
		case Control::FindNext: ctl_find_next(ctx); break;
		case Control::Undo: ctl_undo(ctx); break;
		case Control::Redo: ctl_redo(ctx); break;
		case Control::DownArrow: ctl_open_next(ctx); break;

		case KEY_DOWN: key_down(false); break;
		case KEY_UP: key_up(false); break;
		case KEY_LEFT: key_left(); break;
		case KEY_RIGHT: key_right(); break;
		case KEY_NPAGE: key_page_down(); break;
		case KEY_PPAGE: key_page_up(); break;
		case KEY_HOME: key_home(); break; // move to beginning of line
		case KEY_END: key_end(); break; // move to end of line
		case KEY_SF: key_down(true); break; // shifted down-arrow
		case KEY_SR: key_up(true); break; // shifted up-arrow
		case KEY_SLEFT: key_shift_left(); break;
		case KEY_SRIGHT: key_shift_right(); break;

		case Control::Tab: key_tab(ctx); break;
		case Control::Enter: key_enter(ctx); break;
		case Control::Return: key_return(ctx); break;
		case Control::Backspace: key_backspace(ctx); break;
		case KEY_DC: key_delete(ctx); break;
		case KEY_BTAB: key_btab(ctx); break;	// shift-tab
		case Control::Escape: key_escape(ctx); break;

		default: {
			if (isprint(ch)) key_insert(ch);
			else ctx.show_result("Unknown control: " + std::to_string(ch));
		} break;
	}
	postprocess(ctx);
	return true;
}

void Editor::View::set_help(UI::HelpBar::Panel &panel) {
	if (!_doc.readonly()) {
		panel.cut();
		panel.copy();
		panel.paste();
	}
	panel.to_line();
	panel.find();
	if (_doc.modified()) panel.save();
	panel.save_as();
	if (_doc.can_redo()) panel.redo();
	if (_doc.can_undo()) panel.undo();
}

void Editor::View::select(UI::Frame &ctx, Range range) {
	_anchor = range.begin();
	_selection = range;
	move_cursor(range.end());
	reveal_cursor();
	ctx.repaint();
}

void Editor::View::postprocess(UI::Frame &ctx) {
	reveal_cursor();
	if (_update.has_dirty()) {
		ctx.repaint();
		set_status(ctx);
	}
}

void Editor::View::paint_line(WINDOW *dest, row_t v, State state) {
	size_t index = v + _scroll.v;
	if (!_update.is_dirty(index)) return;
	wmove(dest, (int)v, 0);
	DisplayLine line(_doc.line(index), _config, _syntax);
	line.paint(dest, _scroll.h, _width, state != State::Inactive);
	if (state == State::Inactive) return;
	if (_selection.empty()) return;
	column_t selbegin = 0;
	unsigned selcount = 0;
	line_t begin_line = _selection.begin().line;
	line_t end_line = _selection.end().line;
	if (begin_line < index && end_line > index) {
		selcount = _width;
	} else if (begin_line < index && end_line == index) {
		selcount = line.column(_selection.end().offset);
	} else if (begin_line == index && end_line > index) {
		selbegin = line.column(_selection.begin().offset);
		selcount = _width - selbegin;
	} else if (begin_line == index && end_line == index) {
		selbegin = line.column(_selection.begin().offset);
		selcount = line.column(_selection.end().offset) - selbegin;
	}
	if (selcount > 0) {
		// DisplayLine should probably be responsible for this, since setting
		// A_REVERSE also clears A_ALTCHARSET, which leaves our tab bullets
		// looking a little strange.
		mvwchgat(dest, v, selbegin, selcount, A_REVERSE, 0, NULL);
	}
}

void Editor::View::reveal_cursor() {
	if (_doc.readonly()) return;
	// If the cursor is on a line which is not on screen, scroll vertically to
	// position the line in the center of the window.
	line_t line = _cursor.line;
	if (line < _scroll.v || (line - _scroll.v) >= _height) {
		// Try to center the viewport over the cursor.
		_scroll.v = (line > _halfheight) ? (line - _halfheight) : 0;
		// Don't scroll so far we reveal empty space.
		_scroll.v = std::min(_scroll.v, _maxscroll);
		_update.all();
	}
	// Try to keep the view scrolled left if possible, but if that would put the
	// cursor offscreen, scroll right by the cursor position plus a few extra.
	position_t pos = to_position(_cursor);
	if (pos.h >= _width) {
		column_t newh = pos.h + 4 - _width;
		if (newh != _scroll.h) {
			_scroll.h = newh;
			_update.all();
		}
	} else if (_scroll.h > 0) {
		_scroll.h = 0;
		_update.all();
	}
}

void Editor::View::update_dimensions(WINDOW *view) {
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

void Editor::View::set_status(UI::Frame &ctx) {
	std::string status = _doc.status();
	if (!_doc.readonly()) {
		if (!status.empty()) status.push_back(' ');
		status.push_back('@');
		// humans use weird 1-based line numbers
		status += std::to_string(1 + _cursor.line);
	}
	ctx.set_status(status);
}

void Editor::View::ctl_cut(UI::Frame &ctx) {
	ctl_copy(ctx);
	delete_selection();
	_doc.commit();
}

void Editor::View::ctl_copy(UI::Frame &ctx) {
	if (_selection.empty()) return;
	std::string clip = _doc.text(_selection);
	ctx.app().set_clipboard(clip);
}

void Editor::View::ctl_paste(UI::Frame &ctx) {
	std::string clip = ctx.app().get_clipboard();
	if (clip.empty()) return;
	replace_selection(clip);
}

void Editor::View::ctl_close(UI::Frame &ctx) {
	if (_doc.readonly() || !_doc.modified()) {
		// no formality needed, we're done
		ctx.app().close_file(_targetpath);
		return;
	}
	// ask the user if they want to save first
	Dialog::Confirmation dialog;
	dialog.text = "You have modified this file. Save changes before closing?";
	dialog.yes = [this](UI::Frame &ctx) {
		// save the file
		_doc.Write(_targetpath);
		ctx.app().close_file(_targetpath);
	};
	dialog.no = [this](UI::Frame &ctx) {
		// just close it
		ctx.app().close_file(_targetpath);
	};
	dialog.show(ctx);
}

void Editor::View::ctl_save(UI::Frame &ctx) {
	if (!_doc.modified()) return;
	if (_targetpath.empty()) {
		ctl_save_as(ctx);
		return;
	}
	save(ctx, _targetpath);
}

void Editor::View::ctl_save_as(UI::Frame &ctx) {
	if (_doc.readonly()) return;
	_doc.commit();
	Dialog::Form dialog;
	dialog.fields = {
		{"Save As", _targetpath, &Path::complete_file}
	};
	dialog.commit = [this](UI::Frame &ctx, Dialog::Form::Result &result) {
		std::string path = result.selected_value;
		if (path.empty()) {
			ctx.show_result("Cancelled");
			return;
		}
		// Write the file to disk at its new location.
		save(ctx, path);
		// Update the editor to point at the new path.
		ctx.app().rename_file(_targetpath, path);
		_targetpath = path;
		ctx.set_title(path);
	};
	dialog.show(ctx);
}

void Editor::View::ctl_toline(UI::Frame &ctx) {
	// illogical as it is, the rest of the world seems to think that it is
	// a good idea for line numbers to start counting at 1, so we will
	// accommodate their perverse desires in the name of compatibility.
	Dialog::Form dialog;
	dialog.fields = {
		{
			"Go to line (of " + std::to_string(_doc.maxline() + 1) + ")",
			std::to_string(_cursor.line + 1)
		}
	};
	dialog.commit = [this](UI::Frame &ctx, Dialog::Form::Result &result) {
		std::string value = result.selected_value;
		if (value.empty()) return;
		long valnum = std::stol(value) - 1;
		size_t index = (valnum >= 0) ? valnum : 0;
		move_cursor(_doc.home(index));
		drop_selection();
		postprocess(ctx);
	};
	dialog.show(ctx);
}

void Editor::View::ctl_find(UI::Frame &ctx) {
	Dialog::Form::Field find;
	find.name = "Find";
	find.value = _find_text;
	Range anchor = _selection;
	find.updater = [this, anchor, &ctx](std::string pattern) {
		this->find(ctx, anchor.begin(), pattern);
	};
	Dialog::Form dialog;
	dialog.fields = {find};
	dialog.commit = [this, anchor](UI::Frame& ctx, Dialog::Form::Result& res) {
		_find_text = res.fields["Find"];
		_replace_text.clear();
		if (_find_text.empty()) return;
		if (this->find(ctx, anchor.begin(), _find_text)) {
			_find_next_action = FindNextAction::Find;
		} else {
			ctx.show_result("\"" + _find_text + "\" not found");
		}
	};
	dialog.cancel = [this, anchor](UI::Frame& ctx) {
		select(ctx, anchor);
	};
	_find_next_action = FindNextAction::Nothing;
	dialog.show(ctx);
}

void Editor::View::ctl_replace(UI::Frame &ctx) {
	Dialog::Form::Field find;
	find.name = "Find";
	find.value = _find_text;
	Range anchor = _selection;
	find.updater = [this, anchor, &ctx](std::string pattern) {
		this->find(ctx, anchor.begin(), pattern);
	};
	Dialog::Form::Field repl;
	repl.name = "Replace";
	repl.value = _replace_text;
	Dialog::Form dialog;
	dialog.fields = {find, repl};
	dialog.commit = [this, anchor](UI::Frame& ctx, Dialog::Form::Result& res) {
		_find_text = res.fields["Find"];
		_replace_text = res.fields["Replace"];
		if (_find_text.empty()) return;
		if (this->find(ctx, anchor.begin(), _find_text)) {
			replace_selection(_replace_text);
			_find_next_action = FindNextAction::Replace;
		} else {
			ctx.show_result("\"" + _find_text + "\" not found");
		}
	};
	dialog.cancel = [this, anchor](UI::Frame& ctx) {
		select(ctx, anchor);
	};
	_find_next_action = FindNextAction::Nothing;
	dialog.show(ctx);
}

void Editor::View::ctl_find_next(UI::Frame &ctx) {
	if (_find_next_action == FindNextAction::Nothing) return;
	if (_find_text.empty() || !find(ctx, _selection.end(), _find_text)) {
		_find_next_action = FindNextAction::Nothing;
		return;
	}
	if (_find_next_action != FindNextAction::Replace) return;
	replace_selection(_replace_text);
}

void Editor::View::ctl_undo(UI::Frame &ctx) {
	move_cursor(_doc.undo(_update));
	drop_selection();
	postprocess(ctx);
}

void Editor::View::ctl_redo(UI::Frame &ctx) {
	move_cursor(_doc.redo(_update));
	drop_selection();
	postprocess(ctx);
}

void Editor::View::ctl_open_next(UI::Frame &ctx) {
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

void Editor::View::key_up(bool extend) {
	if (_doc.readonly()) {
		_scroll.v -= std::min(_scroll.v, 1U);
		_update.all();
		return;
	}
	// Move up the screen by the specified number of rows, stopping when we
	// reach zero. If the cursor was already positioned on the top row, move
	// the cursor left to the beginning of the line.
	if (_cursor.line) {
		location_t dest{_cursor.line - 1, _cursor.offset};
		move_cursor(dest);
	} else {
		move_cursor(_doc.home());
	}
	adjust_selection(extend);
}

void Editor::View::key_down(bool extend) {
	if (_doc.readonly()) {
		_scroll.v = std::min(_scroll.v + 1, _maxscroll);
		_update.all();
		return;
	}
	// Move to the next row down the screen, stopping at the maximum row. 
	// If the cursor was already positioned on the maximum row, move the cursor
	// right to the end of the line.
	if (_cursor.line < _doc.maxline()) {
		location_t dest{_cursor.line + 1, _cursor.offset};
		move_cursor(dest);
	} else {
		move_cursor(_doc.end());
	}
	adjust_selection(extend);
}

void Editor::View::key_left() {
	if (_doc.readonly()) return;
	move_cursor(_doc.prev_char(_cursor));
	drop_selection();
}

void Editor::View::key_right() {
	if (_doc.readonly()) return;
	move_cursor(_doc.next_char(_cursor));
	drop_selection();
}

void Editor::View::key_shift_left() {
	if (_doc.readonly()) return;
	move_cursor(_doc.prev_char(_cursor));
	_selection.reset(_anchor, _cursor);
}

void Editor::View::key_shift_right() {
	if (_doc.readonly()) return;
	move_cursor(_doc.next_char(_cursor));
	_selection.reset(_anchor, _cursor);
}

void Editor::View::key_page_up() {
	// move the cursor to the last line of the previous page
	move_cursor(_doc.home(_scroll.v - std::min(_scroll.v, 1U)));
	drop_selection();
}

void Editor::View::key_page_down() {
	// move the cursor to the first line of the next page
	move_cursor(_doc.home(_scroll.v + _height));
	drop_selection();
}

void Editor::View::key_home() {
	move_cursor(_doc.home(_cursor.line));
	drop_selection();
}

void Editor::View::key_end() {
	move_cursor(_doc.end(_cursor.line));
	drop_selection();
}

void Editor::View::delete_selection() {
	if (_selection.empty()) return;
	_update.forward(_selection.begin());
	move_cursor(_doc.erase(_selection));
	drop_selection();
}

void Editor::View::replace_selection(std::string clip) {
	delete_selection();
	location_t oldloc = _cursor;
	location_t newloc = _doc.insert(oldloc, clip);
	if (oldloc.line != newloc.line) {
		_update.forward(oldloc);
	}
	move_cursor(newloc);
	drop_selection();
	_doc.commit();
}

void Editor::View::key_insert(char ch) {
	delete_selection();
	move_cursor(_doc.insert(_cursor, ch));
	_anchor = _cursor;
	_selection.reset(_anchor);
}

void Editor::View::key_tab(UI::Frame &ctx) {
	if (_selection.empty()) {
		// move the cursor forward to the next tab stop, using either a tab
		// character or a series of spaces, as the user requires
		if (_config.indent_with_tabs()) {
			key_insert('\t');
		} else do {
			key_insert(' ');
		} while (0 != _cursor.offset % _config.indent_size());
	} else {
		// indent all lines touched by the selection one more tab then extend
		// the selection to encompass all of those lines, because that's what
		// VS does and I like it that way.
		line_t begin = _selection.begin().line;
		line_t end = _selection.end().line;
		if (end > begin && 0 == _selection.end().offset) end--;
		std::string indent_spaces(_config.indent_size(), ' ');
		for (line_t index = begin; index <= end; ++index) {
			if (_config.indent_with_tabs()) {
				_doc.insert(_doc.home(index), '\t');
			} else {
				_doc.insert(_doc.home(index), indent_spaces);
			}
		}
		_anchor = _doc.home(begin);
		move_cursor(_doc.end(end));
		_selection.reset(_anchor, _cursor);
		_update.range(_selection);
	}
}

void Editor::View::key_btab(UI::Frame &ctx) {
	// Shift-tab unindents the selection, if present, or simply the current
	// line if there is no selection.
	// Remove the leftmost tab character or indent-sized sequence of spaces
	// from each of the selected lines, then extend the selection to encompass
	// all of those lines.
	std::string spaceindent(' ', _config.indent_size());
	line_t begin = _selection.begin().line;
	line_t end = _selection.end().line;
	if (end > begin && 0 == _selection.end().offset) end--;
	for (line_t index = begin; index <= end; ++index) {
		std::string text = _doc.line(index);
		if (text.empty()) continue;
		location_t pretab = _doc.home(index);
		location_t posttab = pretab;
		if ('\t' == text.front()) {
			posttab = _doc.next_char(pretab);
		} else for (auto ch: text) {
			if (' ' != ch) break;
			if (posttab.offset >= _config.indent_size()) break;
			posttab.offset++;
		}
		Range indent(pretab, posttab);
		if (!indent.empty()) {
			_doc.erase(indent);
		}
	}
	_anchor = _doc.home(begin);
	move_cursor(_doc.end(end));
	_selection.reset(_anchor, _cursor);
	_update.range(_selection);
}

void Editor::View::key_escape(UI::Frame &ctx) {
	drop_selection();
}

void Editor::View::key_enter(UI::Frame &ctx) {
	// Split the line at the cursor position, but don't move the cursor.
	delete_selection();
	_doc.split(_cursor);
	_update.forward(_cursor);
}

void Editor::View::key_return(UI::Frame &ctx) {
	// Split the line at the cursor position and move the cursor to the new line.
	delete_selection();
	line_t old_index = _cursor.line;
	move_cursor(_doc.split(_cursor));
	// Add whatever string of whitespace characters begins the previous line.
	for (char ch: _doc.line(old_index)) {
		if (!isspace(ch)) break;
		key_insert(ch);
	}
	_update.forward(_cursor);
}

void Editor::View::key_backspace(UI::Frame &ctx) {
	if (_selection.empty()) {
		move_cursor(_doc.erase(Range(_doc.prev_char(_cursor), _cursor)));
		_update.forward(_cursor);
	} else {
		delete_selection();
	}
}

void Editor::View::key_delete(UI::Frame &ctx) {
	if (_selection.empty()) {
		move_cursor(_doc.erase(Range(_cursor, _doc.next_char(_cursor))));
		_update.forward(_cursor);
	} else {
		delete_selection();
	}
}

void Editor::View::drop_selection() {
	// The selection is no longer interesting. Move the anchor to the
	// current cursor location and reset the selection around it.
	_update.range(_selection);
	_anchor = _cursor;
	_selection.reset(_anchor);
}

void Editor::View::adjust_selection(bool extend) {
	if (extend) {
		// The cursor has moved in range-selection mode.
		// Leave the anchor where it is, then extend the
		// selection to include the new cursor point.
		_selection.reset(_anchor, _cursor);
	} else {
		// The cursor moved but did not extend the selection.
		drop_selection();
	}
}

void Editor::View::move_cursor(location_t loc) {
	// Place the cursor at an absolute document location.
	_update.at(_cursor);
	_cursor = loc;
	// We have updated the cursor's location in the document.
	// Tell the viewer what to redraw, then update the display
	// position according to the new location.
	_update.at(_cursor);
}

Editor::position_t Editor::View::to_position(const location_t &loc) {
	// Compute the screen position for this document location.
	position_t out;
	out.v = std::min(_doc.maxline(), loc.line);
	DisplayLine line(_doc.line(loc.line), _config, _syntax);
	out.h = line.column(loc.offset);
	return out;
}

void Editor::View::save(UI::Frame &ctx, std::string dest) {
	_doc.commit();
	_doc.Write(dest);
	_config.load(dest);
	ctx.set_status(_doc.status());
	std::string stat = "Wrote " + std::to_string(_doc.maxline()+1);
	stat += (_doc.maxline() >= 1) ? " lines" : " line";
	ctx.show_result(stat);
}

bool Editor::View::find(
		UI::Frame &ctx, location_t anchor, std::string pattern) {
	// Look for the pattern after the anchor point.
	Range match = _doc.find(pattern, anchor);
	if (match.empty()) {
		// Look for the pattern after the beginning of the document.
		match = _doc.find(pattern, _doc.home());
		if (match.empty()) {
			// The document does not contain any instances of the pattern.
			match = Range(anchor, anchor);
		}
	}
	select(ctx, match);
	return !match.empty();
}
