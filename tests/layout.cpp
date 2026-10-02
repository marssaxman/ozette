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
#include "text/layout.h"
#include "utf8_locale.h"
#include <limits>
#include <random>

namespace {
void check_scans(const std::string &text, unsigned tab_width) {
	Text::LineLayout layout(text, tab_width);
	for (size_t offset = 0; offset <= text.size() + 1; ++offset) {
		CAPTURE(offset);
		CHECK(Text::column_at(text, offset, tab_width) == layout.column(offset));
	}
	for (unsigned column = 0; column <= layout.width() + 1; ++column) {
		CAPTURE(column);
		CHECK(Text::offset_at(text, column, tab_width) == layout.offset(column));
	}
	CHECK(Text::column_at(text, std::numeric_limits<size_t>::max(), tab_width) == layout.width());
	CHECK(Text::offset_at(text, std::numeric_limits<unsigned>::max(), tab_width) == text.size());
}
} // namespace

TEST_CASE("tabs advance to the next stop from every surrounding column") {
	for (unsigned width: {1, 2, 3, 4, 8, 256}) {
		for (unsigned prefix = 0; prefix <= width * 2; ++prefix) {
			CAPTURE(width);
			CAPTURE(prefix);
			Text::LineLayout layout(std::string(prefix, 'a') + "\tx", width);
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
	Text::LineLayout layout("A\xc3\xa9\xe7\x95\x8c" "e\xcc\x81\tZ", 4);
	const unsigned columns[] = {0, 1, 1, 2, 2, 2, 4, 5, 5, 5, 8, 9};
	for (size_t i = 0; i < sizeof(columns) / sizeof(*columns); ++i) {
		CHECK(layout.column(i) == columns[i]);
		CHECK(Text::column_at("A\xc3\xa9\xe7\x95\x8c" "e\xcc\x81\tZ", i, 4) == columns[i]);
	}
	const size_t offsets[] = {0, 1, 3, 3, 6, 9, 9, 9, 10, 11};
	for (unsigned i = 0; i < sizeof(offsets) / sizeof(*offsets); ++i) {
		CHECK(layout.offset(i) == offsets[i]);
		CHECK(Text::offset_at("A\xc3\xa9\xe7\x95\x8c" "e\xcc\x81\tZ", i, 4) == offsets[i]);
	}
	CHECK(layout.width() == 9);
	CHECK(layout.offset(std::numeric_limits<unsigned>::max()) == 11);
	CHECK(layout.column(std::numeric_limits<size_t>::max()) == 9);
}

TEST_CASE("unprintable and malformed bytes have stable display cells") {
	TestLocale locale;
	const std::string bytes = std::string("\xcc\x81\t\xcc\x81") +
		std::string("\0\r\x1b", 3) + "\x80\xe2\x82";
	Text::LineLayout layout(bytes, 4);
	CHECK(layout.characters().front().value == '?');
	CHECK(layout.characters()[2].value == '?');
	CHECK(layout.width() == 11);
}

TEST_CASE("empty layouts and a zero tab width remain bounded") {
	Text::LineLayout empty("", 4);
	CHECK(empty.width() == 0);
	CHECK(empty.column(42) == 0);
	CHECK(empty.offset(42) == 0);
	CHECK(empty.characters().empty());
	Text::LineLayout tab("\t", 0);
	CHECK(tab.width() == 1);
	CHECK(tab.offset(0) == 0);
	CHECK(tab.offset(1) == 1);
}

TEST_CASE("layouts use bounded replacements when the locale cannot display Unicode") {
	TestLocale locale;
	REQUIRE(setlocale(LC_CTYPE, "C") != nullptr);
	Text::LineLayout layout("\xc3\xa9\xe7\x95\x8c", 4);
	CHECK(layout.width() == 2);
	CHECK(layout.characters()[0].value == '?');
	CHECK(layout.characters()[1].value == '?');
	CHECK(layout.offset(1) == 2);
	CHECK(Text::column_at("\xc3\xa9\xe7\x95\x8c", 2, 4) == 1);
	CHECK(Text::offset_at("\xc3\xa9\xe7\x95\x8c", 1, 4) == 2);
	check_scans("e\xcc\x81\t\xe7\x95\x8c", 4);
}

TEST_CASE("streaming column lookups agree with layouts at byte and cell boundaries") {
	TestLocale locale;
	const std::string samples[] = {
		"", "ascii", "\tX\t", "abc\tZ", "\xc3\xa9\xe7\x95\x8c",
		"e\xcc\x81\xcc\x88X", "e\xcc\x81", "\xcc\x81\t\xcc\x81",
		"\xf0\x9f\x98\x80X", "\xe2\x82", "\xf4\x8f\xbf\xbf",
		std::string("\0\r\x1b\x80", 4),
	};
	for (const auto &text: samples) {
		CAPTURE(text);
		for (unsigned tab_width: {0, 1, 4, 8, 256}) {
			CAPTURE(tab_width);
			check_scans(text, tab_width);
		}
	}
}

TEST_CASE("streaming column lookups preserve mappings in deterministic byte mixtures") {
	TestLocale locale;
	for (unsigned seed = 0; seed < 16; ++seed) {
		CAPTURE(seed);
		std::mt19937 random(seed);
		std::string text;
		for (unsigned i = 0; i < 64; ++i) text += char(random() & 255);
		check_scans(text, seed % 8);
	}
}
