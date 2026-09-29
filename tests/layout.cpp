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
#include "editor/layout.h"
#include "editor/document.h"
#include "utf8_locale.h"
#include <limits>

TEST_CASE("tabs advance to the next stop from every surrounding column") {
	for (unsigned width: {1, 2, 3, 4, 8, 256}) {
		for (unsigned prefix = 0; prefix <= width * 2; ++prefix) {
			CAPTURE(width);
			CAPTURE(prefix);
			Editor::LineLayout layout(std::string(prefix, 'a') + "\tx", width);
			unsigned stop = prefix + width - prefix % width;
			CHECK(layout.column(prefix) == prefix);
			CHECK(layout.column(prefix + 1) == stop);
			CHECK(layout.width() == stop + 1);
			for (unsigned column = prefix; column < stop; ++column) {
				CHECK(layout.offset(column) == prefix);
			}
			CHECK(layout.offset(stop) == prefix + 1);
		}
	}
}

TEST_CASE("line layout maps multibyte wide and combining characters") {
	TestLocale locale;
	// A, e-acute, CJK wide character, e plus combining acute, tab, Z.
	Editor::LineLayout layout("A\xc3\xa9\xe7\x95\x8c" "e\xcc\x81\tZ", 4);
	const unsigned columns[] = {0, 1, 1, 2, 2, 2, 4, 5, 5, 5, 8, 9};
	for (size_t i = 0; i < sizeof(columns) / sizeof(*columns); ++i) {
		CHECK(layout.column(i) == columns[i]);
	}
	const size_t offsets[] = {0, 1, 3, 3, 6, 9, 9, 9, 10, 11};
	for (unsigned i = 0; i < sizeof(offsets) / sizeof(*offsets); ++i) {
		CHECK(layout.offset(i) == offsets[i]);
	}
	CHECK(layout.width() == 9);
	CHECK(layout.offset(std::numeric_limits<unsigned>::max()) == 11);
	CHECK(layout.column(std::numeric_limits<size_t>::max()) == 9);
}

TEST_CASE("unprintable and malformed bytes have stable display cells") {
	TestLocale locale;
	const std::string bytes = std::string("\xcc\x81\t\xcc\x81") +
		std::string("\0\r\x1b", 3) + "\x80\xe2\x82";
	Editor::LineLayout layout(bytes, 4);
	CHECK(layout.characters().front().value == '?');
	CHECK(layout.characters()[2].value == '?');
	CHECK(layout.width() == 11);
	Editor::Document doc;
	doc.insert(doc.home(), bytes);
	for (auto loc = doc.home(); loc != doc.end(); loc = doc.next_char(loc)) {
		unsigned column = layout.column(loc.offset);
		CHECK(layout.offset(column) == loc.offset);
	}
	CHECK(doc.line(0) == bytes);
}

TEST_CASE("empty layouts and a zero tab width remain bounded") {
	Editor::LineLayout empty("", 4);
	CHECK(empty.width() == 0);
	CHECK(empty.column(42) == 0);
	CHECK(empty.offset(42) == 0);
	CHECK(empty.characters().empty());
	Editor::LineLayout tab("\t", 0);
	CHECK(tab.width() == 1);
	CHECK(tab.offset(0) == 0);
	CHECK(tab.offset(1) == 1);
}
