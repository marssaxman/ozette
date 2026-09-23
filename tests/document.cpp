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
#include "editor/document.h"

TEST_CASE("multiline insertion preserves the surrounding text") {
	Editor::Document doc;
	doc.insert(doc.home(), std::string("prefixsuffix"));

	auto cursor = doc.insert({0, 6}, std::string("one\ntwo"));

	CHECK(doc.maxline() == 1);
	CHECK(doc.line(0) == "prefixone");
	CHECK(doc.line(1) == "twosuffix");
	CHECK(cursor.line == 1);
	CHECK(cursor.offset == 3);
	CHECK(doc.text(Editor::Range(doc.home(), doc.end())) == "prefixone\ntwosuffix");
}

TEST_CASE("erasing across lines joins the remaining text") {
	Editor::Document doc;
	doc.insert(doc.home(), std::string("alpha\nbeta\ngamma"));

	auto cursor = doc.erase(Editor::Range({0, 2}, {2, 2}));

	CHECK(doc.maxline() == 0);
	CHECK(doc.line(0) == "almma");
	CHECK(cursor.line == 0);
	CHECK(cursor.offset == 2);
}

TEST_CASE("undo and redo restore an inserted character") {
	Editor::Document doc;
	doc.insert(doc.home(), std::string("abc"));
	doc.commit();
	doc.insert(doc.end(), 'd');
	REQUIRE(doc.line(0) == "abcd");

	Editor::Update update;
	doc.undo(update);
	CHECK(doc.line(0) == "abc");
	REQUIRE(doc.can_redo());

	doc.redo(update);
	CHECK(doc.line(0) == "abcd");
	CHECK_FALSE(doc.can_redo());
}

TEST_CASE("undo and redo restore a multiline deletion") {
	Editor::Document doc;
	const std::string original = "alpha\nbeta\ngamma";
	doc.insert(doc.home(), original);
	doc.commit();
	doc.erase(Editor::Range({0, 2}, {2, 2}));
	REQUIRE(doc.line(0) == "almma");

	Editor::Update update;
	doc.undo(update);
	CHECK(doc.text(Editor::Range(doc.home(), doc.end())) == original);
	REQUIRE(doc.can_redo());

	doc.redo(update);
	CHECK(doc.maxline() == 0);
	CHECK(doc.line(0) == "almma");
}

TEST_CASE("UTF-8 navigation steps across whole code points") {
	Editor::Document doc;
	doc.insert(doc.home(), std::string("A\xc3\xa9\xe2\x82\xac\xf0\x9f\x8c\xb2"));
	const char32_t codepoints[] = {0x41, 0xe9, 0x20ac, 0x1f332};
	const size_t offsets[] = {0, 1, 3, 6, 10};

	for (size_t i = 0; i < 4; ++i) {
		CAPTURE(i);
		Editor::location_t cursor(0, offsets[i]);
		CHECK(doc.codepoint(cursor) == codepoints[i]);
		auto next = doc.next_char(cursor);
		CHECK(next.line == 0);
		CHECK(next.offset == offsets[i + 1]);
		CHECK(doc.prev_char(next).offset == offsets[i]);
	}
}
