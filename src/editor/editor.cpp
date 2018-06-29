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
	position_t cpos;
	cpos.v = std::min(_doc.maxline(), _cursor.line);
	cpos.h = column(_cursor);
	cpos.v -= std::min(cpos.v, _scroll.v);
	cpos.h -= std::min(cpos.h, _scroll.h);
	wmove(dest, cpos.v, cpos.h);
	bool show_cursor = (state == State::Focused) && _selection.empty();
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

		case KEY_DOWN: move_cursor(arrow_down()); break;
		case KEY_UP: move_cursor(arrow_up()); break;
		case KEY_LEFT: move_cursor(arrow_left()); break;
		case KEY_RIGHT: move_cursor(arrow_right()); break;
		case KEY_NPAGE: move_cursor(page_down()); break;
		case KEY_PPAGE: move_cursor(page_up()); break;
		case KEY_HOME: move_cursor(_doc.home(_cursor)); break;
		case KEY_END: move_cursor(_doc.end(_cursor)); break;
		case KEY_SF: extend_selection(arrow_down()); break;
		case KEY_SR: extend_selection(arrow_up()); break;
		case KEY_SLEFT: extend_selection(arrow_left()); break;
		case KEY_SRIGHT: extend_selection(arrow_right()); break;

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
	panel.cut();
	panel.copy();
	panel.paste();
	panel.to_line();
	panel.find();
	if (_doc.modified()) panel.save();
	panel.save_as();
	if (_doc.can_redo()) panel.redo();
	if (_doc.can_undo()) panel.undo();
}

void Editor::View::select(UI::Frame &ctx, Range range) {
	_update.range(_selection);
	_anchor = range.begin();
	_cursor = range.end();
	_selection = range;
	_update.range(range);
	postprocess(ctx);
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
	const std::string &text = _doc.line(index);

	std::vector<int> style(text.size());
	static Regex trailing_space("[[:space:]]+$");
	Regex::Match m = trailing_space.find(text);
	if (!m.empty()) {
		for (size_t i= m.begin; i < m.end; ++i) {
			style[i] = UI::Colors::error();
		}
	}
	for (auto &token: Syntax::parse(_syntax, text)) {
		for (size_t i = token.begin; i < token.end; ++i) {
			style[i] = token.style();
		}
	}

	bool active = state != State::Inactive;
	unsigned hoff = _scroll.h;
	column_t h = 0;
	unsigned width = _width + hoff;
	size_t style_index = 0;
	for (char ch: text) {
		if (h == width) break;
		if (active) {
			wattrset(dest, style[style_index++]);
		}
		// If it's a normal character, just draw it. If it's a tab, draw a
		// bullet, then add spaces up til the next tab stop.
		if (ch != '\t') {
			if (h >= hoff) waddch(dest, ch);
			h++;
		} else {
			chtype bullet = ACS_BULLET;
			do {
				if (h >= hoff) waddch(dest, bullet);
				h++;
				bullet = ' ';
			} while (h < width && 0 != h % _config.indent_size());
		}
	}
	wattrset(dest, UI::Colors::content(active));
	if (h < width) {
		wclrtoeol(dest);
	}

	if (!active) return;
	if (_selection.empty()) return;
	column_t selbegin = 0;
	unsigned selcount = 0;
	line_t begin_line = _selection.begin().line;
	line_t end_line = _selection.end().line;
	if (begin_line < index && end_line > index) {
		selcount = _width;
	} else if (begin_line < index && end_line == index) {
		selcount = column(_selection.end());
	} else if (begin_line == index && end_line > index) {
		selbegin = column(_selection.begin());
		selcount = _width - selbegin;
	} else if (begin_line == index && end_line == index) {
		selbegin = column(_selection.begin());
		selcount = column(_selection.end()) - selbegin;
	}
	if (selcount > 0) {
		// DisplayLine should probably be responsible for this, since setting
		// A_REVERSE also clears A_ALTCHARSET, which leaves our tab bullets
		// looking a little strange.
		mvwchgat(dest, v, selbegin, selcount, A_REVERSE, 0, NULL);
	}
}

void Editor::View::reveal_cursor() {
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
	column_t col = column(_cursor);
	if (col >= _width) {
		// The scroll increment is only coincidentally equal to the default
		// indent size; this does not need to be configurable.
		column_t newh = col + 4 - _width;
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
	if (!status.empty()) status.push_back(' ');
	status.push_back('@');
	// humans use weird 1-based line numbers
	status += std::to_string(1 + _cursor.line);
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
	if (!_doc.modified()) {
		// no formality needed, we're done
		ctx.app().close_file(_targetpath);
		return;
	}
	// ask the user if they want to save first
	Dialog::Confirmation dialog;
	dialog.text = "You have modified this file. Save changes before closing?";
	dialog.yes = [this](UI::Frame &ctx) {
		// attempt to save, close if successful
		if (save(ctx, _targetpath)) {
			ctx.app().close_file(_targetpath);
		}
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
		if (save(ctx, path)) {
			// Update the editor to point at the new path.
			ctx.app().rename_file(_targetpath, path);
			_targetpath = path;
			_config.load(_targetpath);
			ctx.set_title(path);
		}
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
}

void Editor::View::ctl_redo(UI::Frame &ctx) {
	move_cursor(_doc.redo(_update));
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

void Editor::View::delete_selection() {
	if (_selection.empty()) return;
	_update.forward(_selection.begin());
	move_cursor(_doc.erase(_selection));
}

Editor::Range Editor::View::replace_selection(std::string clip) {
	delete_selection();
	location_t oldloc = _cursor;
	location_t newloc = _doc.insert(oldloc, clip);
	if (oldloc.line != newloc.line) {
		_update.forward(oldloc);
	}
	move_cursor(newloc);
	_doc.commit();
	return Range(oldloc, newloc);
}

void Editor::View::key_insert(char ch) {
	delete_selection();
	move_cursor(_doc.insert(_cursor, ch));
}

void Editor::View::key_tab(UI::Frame &ctx) {
	if (_selection.empty()) {
		// move the cursor forward to the next tab stop, using either a tab
		// character or a series of spaces, as the user requires
		do {
			key_insert(_config.indent_style());
		} while (0 != column(_cursor) % _config.indent_size());
	} else {
		location_t begin = _doc.home(_selection.begin());
		location_t end = _doc.end(_selection.end());
		select(ctx, Range(begin, end));
		std::string text = _doc.text(_selection);
		// Add an additional indent to the beginning of each selected line.
		// Replace the selection with the new text. We do this as a single
		// replacement operation so that it can be undone as a single action
		// instead of a series of line-by-line adjustments.
		char indent_char = _config.indent_style();
		size_t indent_count = ('\t' == indent_char)? 1: _config.indent_size();
		size_t offset = 0;
		while (offset != std::string::npos && offset < text.size()) {
			text.insert(offset, indent_count, indent_char);
			offset = text.find_first_of("\n\r", offset);
			offset = text.find_first_not_of("\n\r", offset);
		}
		_selection = replace_selection(text);
	}
}

void Editor::View::key_btab(UI::Frame &ctx) {
	// Shift-tab unindents the selection, if present, or simply the current
	// line if there is no selection.
	// Remove the leftmost tab character or indent-sized sequence of spaces
	// from each of the selected lines, then extend the selection to encompass
	// all of those lines.
	location_t begin = _doc.home(_selection.begin());
	location_t end = _doc.end(_selection.end());
	select(ctx, Range(begin, end));
	std::string text = _doc.text(_selection);
	size_t offset = 0;
	while (offset != std::string::npos && offset < text.size()) {
		size_t munch = offset;
		while (text[munch] == ' ' && munch < text.size()) {
			if (munch - offset >= _config.indent_size()) break;
			++munch;
		}
		if ('\t' == text[offset]) ++munch;
		if (munch > offset) {
			text.erase(offset, munch - offset);
		}
		offset = text.find_first_of("\n\r", offset);
		offset = text.find_first_not_of("\n\r", offset);
	}
	_selection = replace_selection(text);
}

void Editor::View::key_escape(UI::Frame &ctx) {
	_update.range(_selection);
	_selection.reset(_anchor = _cursor);
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
	if (_selection.empty()) extend_selection(arrow_left());
	delete_selection();
}

void Editor::View::key_delete(UI::Frame &ctx) {
	if (_selection.empty()) extend_selection(arrow_right());
	delete_selection();
}

void Editor::View::move_cursor(location_t loc) {
	// Place the cursor at an absolute document location, dropping the
	// selection if one previously existed.
	_update.range(_selection);
	_update.at(loc);
	_selection.reset(_anchor = _cursor = loc);
}

void Editor::View::extend_selection(location_t loc) {
	// Select everything from the anchor to the new location, which becomes
	// the new cursor position.
	_update.at(_cursor);
	_update.at(loc);
	_cursor = loc;
	_selection.reset(_anchor, _cursor);
}

Editor::column_t Editor::View::column(location_t loc) {
	// On which screen column does the character at this location appear?
	column_t col = 0;
	for (location_t i = _doc.home(loc); i < loc; i = _doc.next_char(i)) {
		++col;
		char32_t ch = _doc.codepoint(i);
		if (ch == '\t') {
			col += (_config.indent_size() - col % _config.indent_size());
		}
	}
	return col;
}

Editor::location_t Editor::View::arrow_up() {
	// Find the corresponding location on the line above the cursor.
	// We are concerned about columns, not characters, so we must consider
	// indentation.
	column_t h = column(_cursor);
	location_t dest = _doc.prev_char(_doc.home(_cursor));
	if (dest.line != _cursor.line) {
		while (column(dest) > h) {
			dest = _doc.prev_char(dest);
		}
	}
	return dest;
}

Editor::location_t Editor::View::arrow_down() {
	// Find the corresponding location on the line following the cursor,
	// taking into account indentation width and the fact that the next line
	// may not be as long as the current one.
	column_t h = column(_cursor);
	location_t dest = _doc.end(_doc.next_char(_doc.end(_cursor)));
	if (dest.line != _cursor.line) {
		while (column(dest) > h) {
			dest = _doc.prev_char(dest);
		}
	}
	return dest;
}

Editor::location_t Editor::View::arrow_left() {
	location_t out = _doc.prev_char(_cursor);
	unsigned h = column(out);
	char32_t ch = _doc.codepoint(out);
	while (ch == ' ' && h % _config.indent_size()) {
		location_t prev = _doc.prev_char(out);
		ch = _doc.codepoint(prev);
		if (ch != ' ') break;
		out = prev;
		h--;
	}
	return out;
}

Editor::location_t Editor::View::arrow_right() {
	location_t out = _doc.next_char(_cursor);
	if (' ' == _doc.codepoint(_cursor)) {
		unsigned h = column(out);
		while (' ' == _doc.codepoint(out) && h++ % _config.indent_size()) {
			out = _doc.next_char(out);
		}
	}
	return out;
}

Editor::location_t Editor::View::page_up() {
	return _doc.home(_scroll.v - std::min(_scroll.v, 1U));
}

Editor::location_t Editor::View::page_down() {
	return _doc.home(_scroll.v + _height);
}

bool Editor::View::save(UI::Frame &ctx, std::string dest) {
	_doc.commit();
	bool good = false;
	try {
		_doc.Write(dest);
		std::string stat = "Wrote " + std::to_string(_doc.maxline()+1);
		stat += (_doc.maxline() >= 1) ? " lines" : " line";
		ctx.show_result(stat);
		good = true;
	} catch (const std::runtime_error &e) {
		ctx.show_result(e.what());
	}
	ctx.set_status(_doc.status());
	return good;
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
