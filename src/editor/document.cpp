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
		std::unique_ptr<Line> line(new StrLine(str));
		_lines.push_back(std::move(line));
	}
	_maxline = _lines.empty()? 0: _lines.size() - 1;
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

std::string Editor::Document::text(Range chars)
{
	assert(false);
	return std::string();
}

void Editor::Document::erase(Range chars)
{
	assert(false);
}

Editor::location_t Editor::Document::insert(location_t loc, char ch)
{
	if (loc.line < _lines.size()) {
		std::string text = _lines[loc.line]->text();
		if (loc.offset >= text.size()) loc.offset = text.size();
		text.insert(loc.offset, 1, ch);
		_lines[loc.line].reset(new StrLine(text));
		loc.offset++;
	} else {
		loc.line = _lines.size();
		_lines.emplace_back(new StrLine(std::string(1, ch)));
		loc.offset = 1;
	}
	return loc;
}


