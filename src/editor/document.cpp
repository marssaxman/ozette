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

#include "editor/document.h"
#include <fstream>
#include <sstream>
#include <assert.h>
#include <sys/stat.h>

Editor::Document::Document(const Config &config):
		_settings(config), _syntax(Syntax::lookup("")) {
}

Editor::Document::Document(std::string path, const Config &config):
		_settings(config), _syntax(Syntax::lookup(path)) {
	_lines.clear();
	_edits.clear();
	_modified = false;
	_read_only = false;
	struct stat sb;
	if (stat(path.c_str(), &sb)) {
		_status = "New";
		_maxline = append_line("");
	} else if (S_ISDIR(sb.st_mode)) {
		_status = "Directory!";
		_read_only = true;
	} else if (!S_ISREG(sb.st_mode)) {
		_status = "Not a file!";
		_read_only = true;
	} else {
		_status.clear();
	}

	std::string str;
	std::ifstream file(path);
	// We will read every file using LF as delimiter. When reading a Windows
	// formatted text file, we will then strip the trailing CR.
	while (std::getline(file, str, '\x0A')) {
		if (str.back() == '\x0D') str.pop_back();
		_maxline = append_line(str);
	}
}

void Editor::Document::Write(std::string path) {
	std::ofstream file(path, std::ios::trunc | std::ios::out);
	for (auto &line: _lines) {
		file << line << std::endl;
	}
	file.close();
	clear_modify();
}

void Editor::Document::View(std::string text) {
	// Replace all lines with the contents of this string.
	_lines.clear();
	_lines.emplace_back("");
	_edits.clear();
	_maxline = 0;
	insert(home(), text);
	_modified = false;
	_read_only = true;
	_status.clear();
}

Editor::location_t Editor::Document::home() {
	return home(0);
}

Editor::location_t Editor::Document::end() {
	return end(_maxline);
}

Editor::location_t Editor::Document::home(line_t index) {
	location_t loc = {std::min(index, _maxline), 0};
	return loc;
}

Editor::location_t Editor::Document::end(line_t index) {
	if (index > _maxline) index = _maxline;
	assert(index < _lines.size());
	location_t loc = {index, _lines[index].size()};
	return loc;
}

Editor::location_t Editor::Document::next(location_t loc) {
	if (loc.offset < _lines[loc.line].size()) {
		loc.offset++;
	} else if (loc.line < _maxline) {
		loc.offset = 0;
		loc.line++;
	}
	return loc;
}

Editor::location_t Editor::Document::prev(location_t loc) {
	if (loc.offset > 0) {
		loc.offset--;
	} else if (loc.line > 0) {
		loc.line--;
		loc.offset = _lines[loc.line].size();
	}
	return loc;
}

Editor::Range Editor::Document::find(std::string needle, location_t loc) {
	do {
		loc.offset = _lines[loc.line].find(needle, loc.offset);
		if (loc.offset != std::string::npos) {
			location_t match = {loc.line, loc.offset + needle.size()};
			return Range(loc, sanitize(match));
		}
		loc.offset = 0;
	} while (loc.line++ < _maxline);
	return Range(end(), end());
}

const std::string &Editor::Document::line(line_t index) const {
	return index < _lines.size()? _lines[index]: _blank;
}

Editor::DisplayLine Editor::Document::display(line_t index) const {
	return DisplayLine(line(index), _settings, _syntax);
}

std::string Editor::Document::text(const Range &span) const {
	std::stringstream out;
	location_t loc = span.begin();
	location_t end = span.end();
	std::string chunk = substr_to_end(loc);
	while (loc.line < end.line) {
		out << chunk << '\n';
		chunk = _lines[++loc.line];
		loc.offset = 0;
	}
	out << chunk.substr(0, end.offset - loc.offset);
	return out.str();
}

Editor::location_t Editor::Document::erase(const Range &chars) {
	if (_lines.empty()) return home();
	if (!attempt_modify()) return chars.begin();
	_edits.erase(chars, text(chars));
	location_t begin = sanitize(chars.begin());
	std::string prefix = substr_from_home(begin);
	location_t end = sanitize(chars.end());
	std::string suffix = substr_to_end(end);
	size_t index = begin.line;
	auto beginter = _lines.begin();
	_lines.erase(beginter + begin.line + 1, beginter + end.line + 1);
	_maxline = _lines.size() - 1;
	update_line(index, prefix + suffix);
	return location_t(index, prefix.size());
}

Editor::location_t Editor::Document::insert(location_t begin, char ch) {
	location_t loc = begin;
	sanitize(loc);
	if (!attempt_modify()) return loc;
	if (loc.line < _lines.size()) {
		std::string text = _lines[loc.line];
		text.insert(loc.offset, 1, ch);
		update_line(loc.line, text);
		loc.offset++;
	} else {
		loc.line = append_line(std::string(1, ch));
		loc.offset = 1;
	}
	_edits.insert(Range(begin, loc));
	return loc;
}

Editor::location_t Editor::Document::insert(location_t cur, std::string text) {
	location_t loc = cur;
	sanitize(loc);
	if (!attempt_modify()) return loc;

	std::string suffix;
	if (loc.line < _lines.size()) {
		// Split this line apart around the insertion point. We will insert
		// the new text in between these halves. We will temporarily delete
		// the suffix from its line, since we're likely to be appending more
		// text for a while, but we'll append the suffix back on at the end.
		suffix = substr_to_end(loc);
		update_line(loc.line, substr_from_home(loc));
	} else {
		loc.line = append_line(std::string());
	}

	// Search the text for linebreaks. Every time we find one, we'll
	// cut all the chars from our search position to the linebreak and
	// append them to the current line. Then we'll insert a new, blank
	// line, which will become the new current line. When we're
	// finally out of linebreaks, we'll concatenate whatever is left
	// of the text with our original suffix and append that to the
	// current line. If there were no linebreaks at all, this will
	// simply be the original line we started on.
	size_t startoff = 0, endoff = 0;
	while ((endoff = text.find('\n', startoff)) != std::string::npos) {
		append_to_line(loc.line++, text.substr(startoff, endoff-startoff));
		insert_line(loc.line, "");
		loc.offset = 0;
		startoff = endoff + 1;
	}

	append_to_line(loc.line, text.substr(startoff, endoff));
	loc.offset = _lines[loc.line].size();
	append_to_line(loc.line, suffix);
	_edits.insert(Range(cur, loc));
	return loc;
}

Editor::location_t Editor::Document::split(location_t loc) {
	if (!attempt_modify()) return loc;
	sanitize(loc);
	_edits.split(loc);
	std::string text = line(loc.line);
	update_line(loc.line, text.substr(0, loc.offset));
	loc.line++;
	insert_line(loc.line, text.substr(loc.offset, std::string::npos));
	loc.offset = 0;
	return loc;
}

std::string Editor::Document::substr_from_home(const location_t &loc) {
	return _lines[loc.line].substr(0, loc.offset);
}

std::string Editor::Document::substr_to_end(const location_t &loc) const {
	std::string text = _lines[loc.line];
	return text.substr(std::min(text.size(), loc.offset), std::string::npos);
}

void Editor::Document::update_line(line_t index, std::string text) {
	if (index < _lines.size()) {
		_lines[index] = text;
	} else {
		_lines.emplace_back(text);
	}
}

void Editor::Document::insert_line(line_t index, std::string text) {
	_lines.emplace(_lines.begin() + index, text);
	_maxline = _lines.size() - 1;
}

Editor::line_t Editor::Document::append_line(std::string text) {
	_maxline = _lines.size();
	_lines.emplace_back(text);
	return _maxline;
}

void Editor::Document::append_to_line(line_t index, std::string suffix) {
	update_line(index, _lines[index] + suffix);
}

void Editor::Document::push_to_line(line_t index, std::string prefix) {
	update_line(index, prefix + _lines[index]);
}

Editor::location_t Editor::Document::sanitize(const location_t &loc) {
	// Verify that this location refers to a real place.
	// Fix it if either of its dimensions would be out-of-bounds.
	line_t index = std::min(loc.line, _lines.size()-1);
	offset_t offset = 0;
	if (!_lines.empty()) {
		offset = std::min(loc.offset, _lines[index].size());
	}
	return location_t(index, offset);
}

void Editor::Document::sanitize(location_t *loc) {
	*loc = sanitize(*loc);
}

bool Editor::Document::attempt_modify() {
	if (!_modified && !_read_only) {
		_modified = true;
		_status = "Modified";
	}
	return _modified;
}

void Editor::Document::clear_modify() {
	if (_modified) {
		_modified = false;
		_status.clear();
	}
}
