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
#include <cstring>
#include <exception>
#include <fstream>
#include <sstream>
#include <assert.h>
#include <sys/stat.h>

Editor::Document::Document(std::string path) {
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
	std::ofstream file;
	file.exceptions(std::ios::failbit);
	try {
		file.open(path, std::ios::trunc | std::ios::out);
		for (auto &line: _lines) {
			file << line << std::endl;
		}
		file.close();
		clear_modify();
	} catch (...) {
		int local_errno = errno;
		std::string err = "Failed to write (" + std::to_string(local_errno);
		err += ": " + std::string(std::strerror(local_errno)) + ")";
		throw std::runtime_error(err);
	}
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

Editor::location_t Editor::Document::next_char(location_t loc) {
	const std::string &text = _lines[loc.line];
	if (loc.offset == text.size()) {
		return (loc.line < _maxline)? home(loc.line + 1): end();
	}
	// If this char begins a multibyte sequence, attempt to consume the number
	// of continuation bytes which ought to follow it.
	char ch = text[loc.offset++];
	unsigned continuations = 0;
	if (0xC0 == (0xE0 & ch)) continuations = 1;
	if (0xE0 == (0xF0 & ch)) continuations = 2;
	if (0xF0 == (0xF8 & ch)) continuations = 3;
	if (0xF8 == (0xFC & ch)) continuations = 4;
	if (0xFC == (0xFE & ch)) continuations = 5;
	while (continuations-- && (0x80 == (text[loc.offset] & 0xC0))) {
		loc.offset++;
	}
	return loc;
}

Editor::location_t Editor::Document::prev_char(location_t loc) {
	if (0 == loc.offset) {
		return (loc.line > 0)? end(loc.line - 1): home();
	}
	// If this byte is a continuation, scan backward until we find a byte which
	// could be the beginning of a character sequence. If this continuation
	// byte could feasibly serve as a member of that sequence, jump back to the
	// beginning of the sequence; otherwise return it on its own, since it is
	// an erroneous character encoding.
	const std::string &text = _lines[loc.line];
	offset_t scan = --loc.offset;
	while (0x80 == (text[scan] & 0xC0)) {
		--scan;
	}
	char ch = text[scan];
	switch (loc.offset - scan) {
		case 1: if (0xC0 == (0xE0 & ch)) loc.offset = scan; break;
		case 2: if (0xE0 == (0xF0 & ch)) loc.offset = scan; break;
		case 3: if (0xF0 == (0xF8 & ch)) loc.offset = scan; break;
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

char32_t Editor::Document::codepoint(location_t loc) const {
	auto iter = _lines[loc.line].begin() + loc.offset;
	char ch = *iter;
	// we assume shorter sequences occur more frequently, and we'll do a quick
	// exit for the most common case, which is a 7-bit ASCII character.
	if (0 == (ch & 0x80)) {
		return ch;
	}
	// Any byte with its high bit set must be part of a multi-byte sequence
	// followed by some number of continuation bytes.
	const char32_t replacement_character = 0xFFFD;
	char32_t out = 0;
	unsigned continuations = 0;
	unsigned minimum = 0;
	if ((ch & 0xE0) == 0xC0) { // two bytes
		out = ch & 0x1F;
		continuations = 1;
		minimum = 0x80;
	} else if ((ch & 0xF) == 0xE0) { // three bytes
		out = ch & 0x0F;
		continuations = 2;
		minimum = 0x800;
	} else if ((ch & 0xF8) == 0xF0) { // four bytes
		out = ch & 0x07;
		continuations = 3;
		minimum = 0x10000;
	} else {
		// all other leading bytes are illegal: it's either an overlong sequence
		// (5 or 6 bytes) or it's an out-of-place continuation character.
		return replacement_character;
	}
	// Look for the continuation characters we expect and decode the full
	// character value.
	while (continuations--) {
		ch = *++iter;
		// detect broken sequences with too few continuation bytes
		if ((ch & 0xC0) != 0x80) return replacement_character;
		out = (out << 6) | (ch & 0x3F);
	}
	// guard against overlong sequences and undefined characters
	return (out >= minimum && out <= 0x10FFFF)? out: replacement_character;
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
	sanitize(&begin);
	location_t loc = begin;
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
	sanitize(&cur);
	location_t loc = cur;
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
	sanitize(&loc);
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
