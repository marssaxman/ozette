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
#include "text/utf8.h"
#include <sstream>
#include <assert.h>

Editor::Document::Document(std::string path) {
	std::string contents = _file.read(path);
	_lines.clear();
	if (!_file.exists()) _status = "New";

	size_t start = 0;
	for (size_t end; (end = contents.find('\n', start)) != std::string::npos;) {
		bool crlf = end > start && contents[end - 1] == '\r';
		_lines.push_back(contents.substr(start, end - start - (crlf? 1: 0)));
		_endings.push_back(crlf? "\r\n": "\n");
		start = end + 1;
	}
	_lines.push_back(contents.substr(start));
	if (!_endings.empty()) _newline = _endings.front();
}

void Editor::Document::Write(std::string path, bool overwrite) {
	std::string text;
	for (size_t i = 0; i < _lines.size(); ++i) {
		text += _lines[i];
		if (i < _endings.size()) text += _endings[i];
	}
	_file.write(path, text, overwrite);
	_edits.mark_saved();
	_status.clear();
}

Editor::location_t Editor::Document::home() {
	return home(0);
}

Editor::location_t Editor::Document::end() {
	return end(maxline());
}

Editor::location_t Editor::Document::home(line_t index) {
	location_t loc = {std::min(index, maxline()), 0};
	return loc;
}

Editor::location_t Editor::Document::end(line_t index) {
	if (index > maxline()) index = maxline();
	assert(index < _lines.size());
	location_t loc = {index, _lines[index].size()};
	return loc;
}

Editor::location_t Editor::Document::next_char(location_t loc) {
	loc = clamp(loc);
	const std::string &text = _lines[loc.line];
	if (loc.offset == text.size()) {
		return (loc.line < maxline())? home(loc.line + 1): end();
	}
	loc.offset += Text::UTF8::decode(text, loc.offset).length;
	return loc;
}

Editor::location_t Editor::Document::prev_char(location_t loc) {
	loc = clamp(loc);
	if (loc.offset == 0) {
		return (loc.line > 0)? end(loc.line - 1): home();
	}
	const std::string &text = _lines[loc.line];
	loc.offset = Text::UTF8::previous(text, loc.offset);
	return loc;
}

Editor::Range Editor::Document::find(std::string needle, location_t loc) {
	loc = clamp(loc);
	do {
		loc.offset = _lines[loc.line].find(needle, loc.offset);
		if (loc.offset != std::string::npos) {
			location_t match = {loc.line, loc.offset + needle.size()};
			return Range(loc, clamp(match));
		}
		loc.offset = 0;
	} while (loc.line++ < maxline());
	return Range(end(), end());
}

const std::string &Editor::Document::line(line_t index) const {
	return index < _lines.size()? _lines[index]: _blank;
}

char32_t Editor::Document::codepoint(location_t loc) const {
	loc = clamp(loc);
	return Text::UTF8::decode(_lines[loc.line], loc.offset).value;
}

std::string Editor::Document::text(const Range &span) const {
	std::stringstream out;
	Range bounded(clamp(span.begin()), clamp(span.end()));
	location_t loc = bounded.begin();
	location_t end = bounded.end();
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
	Range span(clamp(chars.begin()), clamp(chars.end()));
	if (span.empty()) return span.begin();
	if (_read_only) return span.begin();
	location_t begin = span.begin();
	std::string prefix = substr_from_home(begin);
	location_t end = span.end();
	std::string suffix = substr_to_end(end);
	_edits.erase(span, text(span),
		{_endings.begin() + begin.line, _endings.begin() + end.line});
	size_t index = begin.line;
	_endings.erase(_endings.begin() + begin.line, _endings.begin() + end.line);
	auto beginter = _lines.begin();
	_lines.erase(beginter + begin.line + 1, beginter + end.line + 1);
	update_line(index, prefix + suffix);
	return location_t(index, prefix.size());
}

Editor::location_t Editor::Document::insert(location_t begin, char ch) {
	if (ch == '\n') return insert(begin, std::string(1, ch));
	begin = clamp(begin);
	if (_read_only) return begin;
	_lines[begin.line].insert(begin.offset, 1, ch);
	location_t loc(begin.line, begin.offset + 1);
	_edits.insert(Range(begin, loc));
	return loc;
}

Editor::location_t Editor::Document::insert(location_t cur, std::string text) {
	return insert(cur, text, {});
}

Editor::location_t Editor::Document::insert(location_t cur, std::string text,
		const std::vector<std::string> &endings) {
	cur = clamp(cur);
	location_t loc = cur;
	if (text.empty()) return loc;
	if (_read_only) return loc;

	// Insert between the two halves of the current line, restoring its suffix
	// after the final inserted line.
	std::string suffix = substr_to_end(loc);
	update_line(loc.line, substr_from_home(loc));

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
	if (!endings.empty()) {
		assert(endings.size() == loc.line - cur.line);
		std::copy(endings.begin(), endings.end(), _endings.begin() + cur.line);
	}
	_edits.insert(Range(cur, loc));
	return loc;
}

Editor::location_t Editor::Document::split(location_t loc) {
	loc = clamp(loc);
	if (_read_only) return loc;
	Edit edit(*this);
	location_t begin = loc;
	std::string text = line(loc.line);
	update_line(loc.line, text.substr(0, loc.offset));
	loc.line++;
	insert_line(loc.line, text.substr(loc.offset, std::string::npos));
	loc.offset = 0;
	_edits.insert(Range(begin, loc));
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
	_lines[index] = std::move(text);
}

void Editor::Document::insert_line(line_t index, std::string text) {
	_endings.insert(_endings.begin() + index - 1, _newline);
	_lines.emplace(_lines.begin() + index, std::move(text));
}

void Editor::Document::append_to_line(line_t index, std::string suffix) {
	_lines[index] += suffix;
}

Editor::location_t Editor::Document::clamp(location_t loc) const {
	loc.line = std::min(loc.line, maxline());
	loc.offset = std::min(loc.offset, _lines[loc.line].size());
	return loc;
}
