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
#include "files.h"

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

TEST_CASE("undo keeps separate deletions at different cursor positions") {
	Editor::Document doc;
	doc.insert(doc.home(), std::string("abcdefgh"));
	doc.commit();
	doc.erase(Editor::Range({0, 0}, {0, 1}));
	doc.erase(Editor::Range({0, 2}, {0, 3}));
	REQUIRE(doc.line(0) == "bcefgh");
	Editor::Update update;
	doc.undo(update);
	CHECK(doc.line(0) == "bcdefgh");
	doc.undo(update);
	CHECK(doc.line(0) == "abcdefgh");
	doc.redo(update);
	CHECK(doc.line(0) == "bcdefgh");
	doc.redo(update);
	CHECK(doc.line(0) == "bcefgh");
}

TEST_CASE("ending a typing group preserves redo until a new edit") {
	Editor::Document doc;
	doc.insert(doc.home(), 'a');
	Editor::Update update;
	doc.undo(update);
	doc.commit();
	REQUIRE(doc.can_redo());
	doc.redo(update);
	CHECK(doc.line(0) == "a");
	doc.undo(update);
	doc.insert(doc.home(), 'b');
	CHECK_FALSE(doc.can_redo());
}

TEST_CASE("undo restores the exact line endings removed by an edit") {
	TempDir dir;
	const std::string original = "alpha\r\nbeta\ngamma\r\n";
	dir.write(original);
	Editor::Document doc(dir.file());
	doc.erase(Editor::Range({0, 2}, {3, 0}));
	Editor::Update update;
	doc.undo(update);
	doc.Write(dir.file());
	CHECK(dir.read() == original);
	doc.redo(update);
	doc.Write(dir.file());
	CHECK(dir.read() == "al");
}

TEST_CASE("nested edit transactions undo replacements and splits together") {
	Editor::Document doc;
	doc.insert(doc.home(), std::string("alpha\nbeta"));
	{
		Editor::Document::Edit edit(doc);
		doc.erase(Editor::Range({0, 2}, {1, 2}));
		auto cursor = doc.insert({0, 2}, std::string("one\ntwo"));
		cursor = doc.split(cursor);
		doc.insert(cursor, std::string("  "));
	}
	const std::string edited = "alone\ntwo\n  ta";
	REQUIRE(doc.text(Editor::Range(doc.home(), doc.end())) == edited);
	Editor::Update update;
	auto cursor = doc.undo(update);
	CHECK(doc.text(Editor::Range(doc.home(), doc.end())) == "alpha\nbeta");
	CHECK(cursor == Editor::location_t(1, 2));
	CHECK(update.is_dirty(2));
	doc.redo(update);
	CHECK(doc.text(Editor::Range(doc.home(), doc.end())) == edited);
}

TEST_CASE("empty edits leave undo and redo alone") {
	Editor::Document doc;
	doc.insert(doc.home(), 'a');
	Editor::Update update;
	doc.undo(update);
	doc.insert(doc.home(), std::string());
	doc.erase(Editor::Range(doc.home(), doc.end()));
	{ Editor::Document::Edit edit(doc); }
	CHECK_FALSE(doc.can_undo());
	CHECK(doc.can_redo());
	CHECK_FALSE(doc.modified());
}

TEST_CASE("undo and redo recognize loaded and saved history positions") {
	TempDir dir;
	dir.write("old");
	Editor::Document doc(dir.file());
	Editor::Update update;
	doc.insert(doc.end(), 'a');
	CHECK(doc.modified());
	CHECK(doc.status() == "Modified");
	doc.undo(update);
	CHECK_FALSE(doc.modified());
	CHECK(doc.status().empty());
	doc.redo(update);
	CHECK(doc.modified());
	doc.Write(dir.file());
	CHECK_FALSE(doc.modified());
	doc.undo(update);
	CHECK(doc.modified());
	doc.redo(update);
	CHECK_FALSE(doc.modified());
	// Saving must also split an otherwise contiguous run of typing.
	doc.insert(doc.end(), 'b');
	doc.undo(update);
	CHECK(doc.line(0) == "olda");
	CHECK_FALSE(doc.modified());
}

TEST_CASE("a new history branch cannot impersonate the saved position") {
	TempDir dir;
	Editor::Document doc;
	Editor::Update update;
	doc.insert(doc.home(), 'a');
	doc.Write(dir.file());
	doc.undo(update);
	doc.insert(doc.home(), 'b');
	CHECK(doc.modified());
	CHECK_FALSE(doc.can_redo());
	doc.undo(update);
	CHECK(doc.modified());
	doc.redo(update);
	CHECK(doc.modified());
	CHECK(doc.line(0) == "b");
}

TEST_CASE("saving after undo retains redo and establishes a new saved position") {
	TempDir dir;
	Editor::Document doc;
	Editor::Update update;
	doc.insert(doc.home(), 'a');
	doc.commit();
	doc.insert(doc.end(), 'b');
	doc.undo(update);
	doc.Write(dir.file());
	REQUIRE(doc.can_redo());
	doc.redo(update);
	CHECK(doc.line(0) == "ab");
	CHECK(doc.modified());
	doc.undo(update);
	CHECK_FALSE(doc.modified());
	doc.undo(update);
	CHECK(doc.modified());
	doc.redo(update);
	CHECK_FALSE(doc.modified());
}

TEST_CASE("a failed save leaves the previous saved position intact") {
	TempDir dir;
	dir.write("old");
	Editor::Document doc(dir.file());
	doc.insert(doc.end(), 'x');
	dir.write("external");
	CHECK_THROWS_AS(doc.Write(dir.file()), Editor::File::Changed);
	CHECK(doc.modified());
	Editor::Update update;
	doc.undo(update);
	CHECK_FALSE(doc.modified());
	doc.redo(update);
	CHECK(doc.modified());
}

TEST_CASE("undoing edits to a new file restores its new status") {
	TempDir dir;
	Editor::Document doc(dir.file());
	doc.insert(doc.home(), 'x');
	Editor::Update update;
	doc.undo(update);
	CHECK_FALSE(doc.modified());
	CHECK(doc.status() == "New");
}
