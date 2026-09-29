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
#include "text/layout.h"
#include "files.h"
#include "utf8_locale.h"
#include <limits>
#include <random>

namespace {
const size_t outside = std::numeric_limits<size_t>::max();

void check_location(Editor::Document &doc, Editor::location_t loc) {
	REQUIRE((loc.line <= doc.maxline()));
	CHECK((loc.offset <= doc.line(loc.line).size()));
}

void check_navigation(Editor::Document &doc) {
	std::vector<Editor::location_t> stops = {doc.home()};
	for (auto loc = doc.home(); loc != doc.end();) {
		auto next = doc.next_char(loc);
		check_location(doc, next);
		REQUIRE((next > loc));
		CHECK((doc.prev_char(next) == loc));
		stops.push_back(next);
		loc = next;
	}
	auto loc = doc.end();
	for (size_t i = stops.size() - 1; i > 0; --i) {
		loc = doc.prev_char(loc);
		CHECK((loc == stops[i - 1]));
	}
	CHECK((doc.prev_char(doc.home()) == doc.home()));
	CHECK((doc.next_char(doc.end()) == doc.end()));
}
} // namespace

TEST_CASE("empty documents always have a navigable first line") {
	TempDir dir;
	Editor::Document doc;
	SUBCASE("untitled buffer") {}
	SUBCASE("missing file") { doc = Editor::Document(dir.file()); }
	SUBCASE("existing empty file") {
		dir.write("");
		doc = Editor::Document(dir.file());
	}
	CHECK((doc.maxline() == 0));
	CHECK((doc.home() == Editor::location_t(0, 0)));
	CHECK((doc.end() == doc.home()));
	CHECK((doc.line(0).empty()));
	CHECK((doc.line(outside).empty()));
	for (auto loc: {Editor::location_t(), {0, outside}, {outside, 0}, {outside, outside}}) {
		CHECK((doc.clamp(loc) == doc.home()));
		CHECK((doc.home(loc) == doc.home()));
		CHECK((doc.end(loc) == doc.home()));
		CHECK((doc.next_char(loc) == doc.home()));
		CHECK((doc.prev_char(loc) == doc.home()));
		CHECK((doc.codepoint(loc) == 0));
		CHECK((doc.find("", loc).empty()));
		CHECK((doc.find("x", loc).empty()));
		CHECK((doc.text(Editor::Range(loc, doc.home())).empty()));
		CHECK((doc.erase(Editor::Range(loc, doc.home())) == doc.home()));
		CHECK((doc.insert(loc, std::string()) == doc.home()));
	}
	CHECK_FALSE(doc.modified());
	doc.Write(dir.file());
	CHECK((dir.read().empty()));
	auto end = doc.insert({outside, outside}, 'x');
	CHECK((end == Editor::location_t(0, 1)));
	doc.erase(Editor::Range(doc.home(), {outside, outside}));
	CHECK((doc.maxline() == 0));
	CHECK((doc.end() == doc.home()));
	Editor::Update update;
	doc.undo(update);
	CHECK((doc.line(0) == "x"));
	doc.redo(update);
	CHECK((doc.end() == doc.home()));
}

TEST_CASE("blank lines and final line boundaries survive navigation and saving") {
	for (auto bytes: {"\n", "\r\n", "\n\n", "\r\n\n\r\n", "\nlast", "first\n"}) {
		CAPTURE(bytes);
		TempDir dir;
		dir.write(bytes);
		Editor::Document doc(dir.file());
		check_navigation(doc);
		for (size_t line = 0; line <= doc.maxline(); ++line) {
			CHECK((doc.codepoint(doc.end(line)) == 0));
			CHECK((doc.end({line, outside}) == doc.end(line)));
		}
		doc.Write(dir.file());
		CHECK((dir.read() == bytes));
		doc.erase(Editor::Range(doc.home(), doc.end()));
		CHECK((doc.maxline() == 0));
		CHECK((doc.end() == doc.home()));
		Editor::Update update;
		doc.undo(update);
		doc.Write(dir.file());
		CHECK((dir.read() == bytes));
	}
}

TEST_CASE("public document operations bound arbitrary byte locations") {
	Editor::Document doc;
	doc.insert(doc.home(), std::string("abc\nxy"));
	doc.commit();
	CHECK((doc.text(Editor::Range({0, 1}, {outside, outside})) == "bc\nxy"));
	CHECK((doc.text(Editor::Range({0, outside}, {1, outside})) == "\nxy"));
	CHECK((doc.find("y", {outside, 0}).begin() == Editor::location_t(1, 1)));
	CHECK((doc.find("x", {outside, outside}).begin() == doc.end()));
	CHECK((doc.codepoint({outside, outside}) == 0));
	CHECK((doc.next_char({0, outside}) == Editor::location_t(1, 0)));
	CHECK((doc.prev_char({outside, outside}) == Editor::location_t(1, 1)));
	SUBCASE("character insertion") {
		CHECK((doc.insert({outside, outside}, 'z') == Editor::location_t(1, 3)));
		CHECK((doc.line(1) == "xyz"));
	}
	SUBCASE("multiline insertion") {
		CHECK((doc.insert({outside, outside}, std::string("q\nr")) == Editor::location_t(2, 1)));
		CHECK((doc.line(1) == "xyq"));
		CHECK((doc.line(2) == "r"));
	}
	SUBCASE("splitting") {
		CHECK((doc.split({outside, outside}) == Editor::location_t(2, 0)));
		CHECK((doc.line(1) == "xy"));
	}
	SUBCASE("deletion") {
		CHECK((doc.erase(Editor::Range({0, 1}, {outside, outside})) == Editor::location_t(0, 1)));
		CHECK((doc.line(0) == "a"));
	}
	Editor::Update update;
	check_location(doc, doc.undo(update));
	CHECK((doc.text(Editor::Range(doc.home(), doc.end())) == "abc\nxy"));
	check_location(doc, doc.redo(update));
	check_navigation(doc);
}

TEST_CASE("inserting a newline character uses the normal line representation") {
	TempDir dir;
	dir.write("ab\r\ncd");
	Editor::Document doc(dir.file());
	CHECK((doc.insert({0, 1}, '\n') == Editor::location_t(1, 0)));
	CHECK((doc.maxline() == 2));
	CHECK((doc.line(0) == "a"));
	CHECK((doc.line(1) == "b"));
	doc.Write(dir.file());
	CHECK((dir.read() == "a\r\nb\r\ncd"));
	Editor::Update update;
	doc.undo(update);
	doc.Write(dir.file());
	CHECK((dir.read() == "ab\r\ncd"));
	doc.redo(update);
	doc.Write(dir.file());
	CHECK((dir.read() == "a\r\nb\r\ncd"));
}

TEST_CASE("UTF-8 decoding accepts scalar boundaries and rejects invalid sequences") {
	struct example { std::string bytes; char32_t value; };
	const example valid[] = {
		{std::string(1, '\0'), 0}, {"\x7f", 0x7f}, {"\xc2\x80", 0x80},
		{"\xdf\xbf", 0x7ff}, {"\xe0\xa0\x80", 0x800}, {"\xed\x9f\xbf", 0xd7ff},
		{"\xee\x80\x80", 0xe000}, {"\xef\xbb\xbf", 0xfeff}, {"\xef\xbf\xbf", 0xffff},
		{"\xf0\x90\x80\x80", 0x10000}, {"\xf4\x8f\xbf\xbf", 0x10ffff}
	};
	for (const auto &item: valid) {
		CAPTURE(item.value);
		Editor::Document doc;
		doc.insert(doc.home(), item.bytes);
		CHECK((doc.codepoint(doc.home()) == item.value));
		CHECK((doc.next_char(doc.home()) == doc.end()));
		CHECK((doc.prev_char(doc.end()) == doc.home()));
		for (size_t offset = 0; offset <= item.bytes.size(); ++offset) {
			check_location(doc, doc.next_char({0, offset}));
			check_location(doc, doc.prev_char({0, offset}));
			if (offset && offset < item.bytes.size()) CHECK((doc.codepoint({0, offset}) == 0xfffd));
		}
	}
	const std::string invalid[] = {
		"\x80", "\xbf", "\xc0\x80", "\xc1\xbf", "\xc2", "\xe2", "\xe2\x82",
		"\xf0", "\xf0\x9f", "\xf0\x9f\x8c", "\xe0\x80\x80", "\xed\xa0\x80",
		"\xed\xbf\xbf", "\xf0\x80\x80\x80", "\xf4\x90\x80\x80", "\xf5\x80\x80\x80",
		"\xf8\x88\x80\x80\x80", "\xfc\x84\x80\x80\x80\x80", "\xfe\xff",
		std::string(4096, '\x80')
	};
	for (const auto &bytes: invalid) {
		Editor::Document doc;
		doc.insert(doc.home(), bytes);
		for (size_t offset = 0; offset < bytes.size(); ++offset) {
			CHECK((doc.codepoint({0, offset}) == 0xfffd));
			CHECK((doc.next_char({0, offset}) == Editor::location_t(0, offset + 1)));
			CHECK((doc.prev_char({0, offset + 1}) == Editor::location_t(0, offset)));
		}
	}
}

TEST_CASE("arbitrary byte sequences have reversible bounded navigation") {
	// Include every pair, then longer deterministic mixtures. This catches
	// isolated continuations, unexpected ASCII/newlines, and adjacent sequences.
	for (unsigned pair = 0; pair < 65536; ++pair) {
		CAPTURE(pair);
		Editor::Document doc;
		doc.insert(doc.home(), std::string({char(pair >> 8), char(pair & 255)}));
		check_navigation(doc);
	}
	for (unsigned seed = 0; seed < 32; ++seed) {
		CAPTURE(seed);
		std::mt19937 random(seed);
		std::string bytes;
		for (unsigned i = 0; i < 128; ++i) bytes += char(random() & 255);
		Editor::Document doc;
		doc.insert(doc.home(), bytes);
		check_navigation(doc);
	}
}

TEST_CASE("malformed bytes survive navigation deletion undo redo and saving") {
	const std::string bytes = std::string("\x80\xbf\xc0\xaf\xe2\x82") +
		"A\xc3\xa9\xed\xa0\x80\r\n\xf4\x90\x80\x80\n\xf0\x9f";
	TempDir dir;
	dir.write(bytes);
	Editor::Document doc(dir.file());
	check_navigation(doc);
	doc.Write(dir.file());
	CHECK((dir.read() == bytes));
	while (doc.end() != doc.home()) {
		doc.commit();
		doc.erase(Editor::Range(doc.prev_char(doc.end()), doc.end()));
	}
	CHECK((doc.maxline() == 0));
	Editor::Update update;
	while (doc.can_undo()) check_location(doc, doc.undo(update));
	doc.Write(dir.file());
	CHECK((dir.read() == bytes));
	while (doc.can_redo()) check_location(doc, doc.redo(update));
	doc.Write(dir.file());
	CHECK((dir.read().empty()));
}

TEST_CASE("document navigation agrees with layout for unprintable and malformed bytes") {
	TestLocale locale;
	const std::string bytes = std::string("\xcc\x81\t\xcc\x81") +
		std::string("\0\r\x1b", 3) + "\x80\xe2\x82";
	Text::LineLayout layout(bytes, 4);
	Editor::Document doc;
	doc.insert(doc.home(), bytes);
	for (auto loc = doc.home(); loc != doc.end(); loc = doc.next_char(loc)) {
		unsigned column = layout.column(loc.offset);
		CHECK(layout.offset(column) == loc.offset);
	}
	CHECK(doc.line(0) == bytes);
}
