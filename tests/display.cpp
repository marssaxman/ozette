// ozette
// Copyright (C) 2026 Mars J. Saxman
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

#include "doctest.h"
#include "editor/editor.h"
#include "ui/text.h"
#include "files.h"
#include "ui.h"
#include "utf8_locale.h"
#include <limits>

namespace {
class TestEditor : public Editor::View {
public:
	using Editor::View::View;
	void draw(WINDOW *dest, State state = State::Focused) { paint_into(dest, state); }
};

struct TestWindow {
	TestWindow(int width): window(newwin(4, width, 0, 0)) {
		if (!window) throw std::runtime_error("newwin failed");
	}
	~TestWindow() { delwin(window); }
	WINDOW *window;
};

std::wstring cell(WINDOW *window, int v, int h) {
	cchar_t value;
	REQUIRE(mvwin_wch(window, v, h, &value) != ERR);
	wchar_t text[CCHARW_MAX + 1] = {};
	attr_t attributes;
	short color;
	REQUIRE(getcchar(&value, text, &attributes, &color, nullptr) != ERR);
	return text;
}

void check_cursor(WINDOW *window, unsigned v, unsigned h) {
	int row, column;
	getyx(window, row, column);
	CHECK(row == static_cast<int>(v));
	CHECK(column == static_cast<int>(h));
}
} // namespace

TEST_CASE("painted tabs and cursor columns agree at every tab stop") {
	TestScreen screen;
	TestFrame frame;
	TempDir dir;
	TestWindow dest(24);
	for (unsigned prefix = 0; prefix <= 8; ++prefix) {
		dir.write(std::string(prefix, 'a') + "\tX");
		TestEditor view(dir.file());
		view.process(frame, KEY_END);
		view.draw(dest.window);
		unsigned stop = prefix + 4 - prefix % 4;
		check_cursor(dest.window, 0, stop + 1);
		CHECK((mvwinch(dest.window, 0, prefix) & A_CHARTEXT) == (ACS_BULLET & A_CHARTEXT));
		for (unsigned column = prefix + 1; column < stop; ++column) {
			CHECK(cell(dest.window, 0, column) == L" ");
		}
		CHECK(cell(dest.window, 0, stop) == L"X");
	}
}

TEST_CASE("painting preserves complete wide and combining characters") {
	TestLocale locale;
	TestScreen screen;
	TestFrame frame;
	TempDir dir;
	TestWindow dest(16);
	dir.write("A\xc3\xa9\xe7\x95\x8c" "e\xcc\x81\tZ");
	TestEditor view(dir.file());
	view.process(frame, KEY_END);
	view.draw(dest.window);
	check_cursor(dest.window, 0, 9);
	CHECK(cell(dest.window, 0, 0) == L"A");
	CHECK(cell(dest.window, 0, 1) == L"\u00e9");
	CHECK(cell(dest.window, 0, 2) == L"\u754c");
	CHECK(cell(dest.window, 0, 4) == L"e\u0301");
	CHECK(cell(dest.window, 0, 8) == L"Z");
}

TEST_CASE("selections are clipped after horizontal scrolling") {
	TestScreen screen;
	TestFrame frame;
	TempDir dir;
	TestWindow dest(8);
	dir.write("0123456789abc");
	TestEditor view(dir.file());
	view.draw(dest.window);
	for (unsigned begin: {0, 5, 9, 10, 13}) {
		CAPTURE(begin);
		view.select(frame, Editor::Range({0, begin}, {0, 13}));
		view.draw(dest.window);
		check_cursor(dest.window, 0, 4);
		CHECK(cell(dest.window, 0, 0) == L"9");
		CHECK(cell(dest.window, 0, 3) == L"c");
		for (unsigned column = 0; column < 8; ++column) {
			bool selected = begin < 13 && column >= (begin > 9? begin - 9: 0) && column < 4;
			CHECK(bool(mvwinch(dest.window, 0, column) & A_REVERSE) == selected);
		}
	}
}

TEST_CASE("multiline selections include visible newline space and skip hidden starts") {
	TestScreen screen;
	TestFrame frame;
	TempDir dir;
	TestWindow dest(8);
	dir.write("0123456789abcdefghijk\nmiddle\nlast");
	TestEditor view(dir.file());
	view.select(frame, Editor::Range({0, 20}, {2, 2}));
	view.draw(dest.window);
	for (int column = 0; column < 8; ++column) {
		CHECK_FALSE((mvwinch(dest.window, 0, column) & A_REVERSE) != 0);
		CHECK((mvwinch(dest.window, 1, column) & A_REVERSE) != 0);
		CHECK(bool(mvwinch(dest.window, 2, column) & A_REVERSE) == (column < 2));
	}
	view.draw(dest.window, UI::View::State::Inactive);
	CHECK_FALSE((mvwinch(dest.window, 1, 0) & A_REVERSE) != 0);
	CHECK((mvwinch(dest.window, 1, 0) & A_DIM) != 0);
}

TEST_CASE("viewport edges do not split wide characters or leak into another row") {
	TestLocale locale;
	TestScreen screen;
	TestWindow dest(8);
	Text::LineLayout layout("\xe7\x95\x8c" "AB\xe7\x95\x8c" "C", 4);
	mvwaddstr(dest.window, 1, 0, "sentinel");
	UI::paint_text(dest.window, 0, 0, 4, layout, 1, A_NORMAL);
	CHECK(cell(dest.window, 0, 0) == L" ");
	CHECK(cell(dest.window, 0, 1) == L"A");
	CHECK(cell(dest.window, 0, 2) == L"B");
	CHECK(cell(dest.window, 0, 3) == L" ");
	CHECK(cell(dest.window, 1, 0) == L"s");
	UI::paint_text(dest.window, 0, 0, 5, layout, 1, A_NORMAL);
	CHECK(cell(dest.window, 0, 3) == L"\u754c");
	CHECK(cell(dest.window, 1, 0) == L"s");
}

TEST_CASE("selected tabs retain their bullet and clipped fragments remain spaces") {
	TestScreen screen;
	TestWindow dest(8);
	Text::LineLayout layout("\tab", 4);
	UI::paint_text(dest.window, 0, 0, 8, layout, 0, A_NORMAL, {}, layout.span(0, 1));
	chtype bullet = mvwinch(dest.window, 0, 0);
	CHECK((bullet & A_REVERSE) != 0);
	CHECK((bullet & A_ALTCHARSET) != 0);
	CHECK((bullet & A_CHARTEXT) == (ACS_BULLET & A_CHARTEXT));
	UI::paint_text(dest.window, 0, 0, 5, layout, 2, A_NORMAL, {}, layout.span(0, 1));
	CHECK(cell(dest.window, 0, 0) == L" ");
	CHECK(cell(dest.window, 0, 1) == L" ");
	CHECK(cell(dest.window, 0, 2) == L"a");
	CHECK((mvwinch(dest.window, 0, 0) & A_REVERSE) != 0);
	CHECK_FALSE((mvwinch(dest.window, 0, 2) & A_REVERSE) != 0);
}

TEST_CASE("selection clipping handles ranges outside either viewport edge") {
	TestScreen screen;
	TestWindow dest(8);
	Text::LineLayout layout("0123456789abcdefghij", 4);
	for (const auto &span: {Text::LineLayout::Span(0, 5), {12, 20}, {0, 20}, {6, 10}}) {
		UI::paint_text(dest.window, 0, 0, 5, layout, 5, A_NORMAL, {}, span);
		for (unsigned column = 0; column < 5; ++column) {
			bool selected = column + 5 >= span.begin && column + 5 < span.end;
			CHECK(bool(mvwinch(dest.window, 0, column) & A_REVERSE) == selected);
		}
	}
}

TEST_CASE("selecting combining marks or interior bytes highlights their complete glyph") {
	TestLocale locale;
	TestScreen screen;
	TestFrame frame;
	TempDir dir;
	TestWindow dest(8);
	dir.write("e\xcc\x81\xe7\x95\x8cZ");
	TestEditor view(dir.file());
	view.select(frame, Editor::Range({0, 1}, {0, 3}));
	view.draw(dest.window);
	CHECK((mvwinch(dest.window, 0, 0) & A_REVERSE) != 0);
	CHECK_FALSE((mvwinch(dest.window, 0, 1) & A_REVERSE) != 0);
	view.select(frame, Editor::Range({0, 4}, {0, 5}));
	view.draw(dest.window);
	CHECK_FALSE((mvwinch(dest.window, 0, 0) & A_REVERSE) != 0);
	CHECK((mvwinch(dest.window, 0, 1) & A_REVERSE) != 0);
	CHECK((mvwinch(dest.window, 0, 2) & A_REVERSE) != 0);
	CHECK_FALSE((mvwinch(dest.window, 0, 3) & A_REVERSE) != 0);
}

TEST_CASE("controls and invalid bytes draw bounded replacements") {
	TestLocale locale;
	TestScreen screen;
	TestWindow dest(8);
	Text::LineLayout layout(std::string("\0\r\x1b\x80x", 5), 4);
	UI::paint_text(dest.window, 0, 0, 8, layout, 0, A_NORMAL);
	for (int column = 0; column < 3; ++column) CHECK(cell(dest.window, 0, column) == L"?");
	CHECK(cell(dest.window, 0, 3) == L"\ufffd");
	CHECK(cell(dest.window, 0, 4) == L"x");
	CHECK(cell(dest.window, 1, 0) == L" ");
}

TEST_CASE("painting takes each character's style from its byte offset") {
	TestLocale locale;
	TestScreen screen;
	TestWindow dest(8);
	Text::LineLayout layout("\xc3\xa9\xe7\x95\x8cX", 4);
	std::vector<int> styles = {A_BOLD, A_NORMAL, A_UNDERLINE, A_NORMAL, A_NORMAL, A_DIM};
	UI::paint_text(dest.window, 0, 0, 8, layout, 0, A_NORMAL, styles);
	CHECK((mvwinch(dest.window, 0, 0) & A_BOLD) != 0);
	CHECK((mvwinch(dest.window, 0, 1) & A_UNDERLINE) != 0);
	CHECK((mvwinch(dest.window, 0, 2) & A_UNDERLINE) != 0);
	CHECK((mvwinch(dest.window, 0, 3) & A_DIM) != 0);
}

TEST_CASE("combining marks at a multiline selection start include their base") {
	TestLocale locale;
	TestScreen screen;
	TestFrame frame;
	TempDir dir;
	TestWindow dest(8);
	dir.write("e\xcc\x81\nnext");
	TestEditor view(dir.file());
	view.select(frame, Editor::Range({0, 1}, {1, 0}));
	view.draw(dest.window);
	CHECK((mvwinch(dest.window, 0, 0) & A_REVERSE) != 0);
	CHECK((mvwinch(dest.window, 0, 7) & A_REVERSE) != 0);
	CHECK_FALSE((mvwinch(dest.window, 1, 0) & A_REVERSE) != 0);
}

TEST_CASE("painting reveals the cursor after resizing to a narrow viewport") {
	TestScreen screen;
	TestFrame frame;
	TempDir dir;
	dir.write("0123456789abc");
	TestEditor view(dir.file());
	view.process(frame, KEY_END);
	for (int width: {8, 3, 2, 1, 16}) {
		TestWindow dest(width);
		view.draw(dest.window);
		check_cursor(dest.window, 0, width > 13? 13: std::max(0, width - 4));
	}
}

TEST_CASE("long combining sequences stay within the terminal cell capacity") {
	TestLocale locale;
	TestScreen screen;
	TestFrame frame;
	TempDir dir;
	TestWindow dest(8);
	std::string text = "abcdefge";
	for (unsigned i = 0; i < 32; ++i) text += "\xcc\x81";
	dir.write(text);
	TestEditor view(dir.file());
	view.draw(dest.window);
	CHECK(cell(dest.window, 0, 7).front() == L'e');
	CHECK(cell(dest.window, 0, 7).size() <= CCHARW_MAX);
	CHECK(cell(dest.window, 1, 0) == L" ");
	view.process(frame, KEY_END);
	view.process(frame, 'X');
	REQUIRE(view.save(frame) == Editor::View::SaveResult::Saved);
	CHECK(dir.read() == text + "X");
}
