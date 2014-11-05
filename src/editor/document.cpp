#include "document.h"
#include <fstream>
#include <assert.h>

namespace {
class BlankLine : public Editor::Line
{
public:
	virtual std::string text() const override { return std::string(); }
	virtual size_t size() const override { return 0; }
};

class StrLine : public Editor::Line
{
public:
	StrLine(std::string text): _text(text) {}
	virtual std::string text() const override { return _text; }
	virtual size_t size() const override { return _text.size(); }
private:
	std::string _text;
};
} // namespace

Editor::Document::Document(std::string targetpath):
	_blank(new BlankLine)
{
	std::string str;
	std::ifstream file(targetpath);
	while (std::getline(file, str)) {
		_maxline = append_line(str);
	}
}

Editor::Line &Editor::Document::line(line_t index)
{
	// Get the line at the specified index.
	// If no such line exists, return a blank.
	return index < _lines.size() ? *_lines[index].get() : *_blank.get();
}

Editor::location_t Editor::Document::home()
{
	location_t loc = {0,0};
	return loc;
}

Editor::location_t Editor::Document::end()
{
	location_t loc = {_maxline, line(_maxline).size()};
	return loc;
}

Editor::position_t Editor::Document::position(const location_t &loc)
{
	// Compute the screen position for this document location.
	position_t out;
	out.v = std::min(_maxline, loc.line);
	out.h = line(loc.line).column(loc.offset);
	return out;
}

Editor::location_t Editor::Document::location(const position_t &loc)
{
	// Locate the character in the document corresponding to the
	// given screen position.
	location_t out;
	out.line = loc.v;
	out.offset = line(out.line).offset(loc.h);
	return out;
}

Editor::location_t Editor::Document::next(location_t loc)
{
	if (loc.offset < line(loc.line).size()) {
		loc.offset++;
	} else if (loc.line < _maxline) {
		loc.offset = 0;
		loc.line++;
	}
	return loc;
}

Editor::location_t Editor::Document::prev(location_t loc)
{
	if (loc.offset > 0) {
		loc.offset--;
	} else if (loc.line > 0) {
		loc.line--;
		loc.offset = line(loc.line).size();
	}
	return loc;
}

Editor::location_t Editor::Document::erase(const Range &chars)
{
	if (_lines.empty()) return home();
	location_t begin = sanitize(chars.begin());
	std::string prefix = _lines[begin.line]->text().substr(0, begin.offset);
	location_t end = sanitize(chars.end());
	std::string suffix = _lines[end.line]->text().substr(end.offset, std::string::npos);
	size_t index = begin.line;
	if (chars.multiline()) {
		auto iter = _lines.begin();
		_lines.erase(iter + begin.line, iter + end.line + 1);
		insert_line(index, prefix + suffix);
	} else {
		update_line(index, prefix + suffix);
	}
	return location_t(index, prefix.size());
}

Editor::location_t Editor::Document::insert(location_t loc, char ch)
{
	sanitize(loc);
	if (loc.line < _lines.size()) {
		std::string text = _lines[loc.line]->text();
		if (loc.offset >= text.size()) loc.offset = text.size();
		text.insert(loc.offset, 1, ch);
		update_line(loc.line, text);
		loc.offset++;
	} else {
		loc.line = append_line(std::string(1, ch));
		loc.offset = 1;
	}
	return loc;
}

Editor::location_t Editor::Document::split(location_t loc)
{
	sanitize(loc);
	std::string text = line(loc.line).text();
	update_line(loc.line, text.substr(0, loc.offset));
	loc.line++;
	insert_line(loc.line, text.substr(loc.offset, std::string::npos));
	loc.offset = 0;
	return loc;
}

void Editor::Document::update_line(line_t index, std::string text)
{
	_lines[index].reset(new StrLine(text));
}

void Editor::Document::insert_line(line_t index, std::string text)
{
	_lines.emplace(_lines.begin() + index, new StrLine(text));
}

Editor::line_t Editor::Document::append_line(std::string text)
{
	line_t index = _lines.size();
	_lines.emplace_back(new StrLine(text));
	return index;
}

Editor::location_t Editor::Document::sanitize(const location_t &loc)
{
	// Verify that this location refers to a real place.
	// Fix it if either of its dimensions would be out-of-bounds.
	line_t index = std::min(loc.line, _lines.size());
	offset_t offset = _lines.empty() ? 0 : std::min(loc.offset, _lines[loc.line]->size());
	return location_t(index, offset);
}

void Editor::Document::sanitize(location_t *loc)
{
	*loc = sanitize(*loc);
}
