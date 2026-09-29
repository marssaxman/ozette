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
#include "files.h"
#include "ui.h"
#include "utf8_locale.h"

TEST_CASE("vertical movement uses the same tab stops as indentation") {
	TestScreen screen;
	TestFrame frame;
	TempDir dir;
	for (unsigned prefix = 0; prefix <= 8; ++prefix) {
		CAPTURE(prefix);
		unsigned stop = prefix + 4 - prefix % 4;
		std::string first = std::string(prefix, 'a') + "\tX";
		std::string second(16, 'b');
		dir.write(first + "\n" + second);
		Editor::View view(dir.file());
		view.select(frame, Editor::Range({0, prefix + 1}, {0, prefix + 1}));
		view.process(frame, KEY_DOWN);
		view.process(frame, '|');
		REQUIRE(view.save(frame) == Editor::View::SaveResult::Saved);
		CHECK(dir.read() == first + "\n" + second.substr(0, stop) + "|" + second.substr(stop));
		view.process(frame, Control::Undo);
		view.process(frame, KEY_UP);
		view.process(frame, '|');
		REQUIRE(view.save(frame) == Editor::View::SaveResult::Saved);
		CHECK(dir.read() == first.substr(0, prefix + 1) + "|X\n" + second);
	}
}

TEST_CASE("vertical movement lands before partial tabs and wide characters") {
	TestLocale locale;
	TestScreen screen;
	TestFrame frame;
	TempDir dir;
	const std::string second = "\t\xe7\x95\x8c" "e\xcc\x81Z";
	const size_t offsets[] = {0, 0, 0, 0, 1, 1, 4, 7, 8, 8};
	for (unsigned column = 0; column < sizeof(offsets) / sizeof(*offsets); ++column) {
		CAPTURE(column);
		std::string first(column, 'a');
		dir.write(first + "\n" + second);
		Editor::View view(dir.file());
		view.process(frame, KEY_END);
		view.process(frame, KEY_DOWN);
		view.process(frame, '|');
		REQUIRE(view.save(frame) == Editor::View::SaveResult::Saved);
		CHECK(dir.read() == first + "\n" + second.substr(0, offsets[column]) +
			"|" + second.substr(offsets[column]));
	}
}

TEST_CASE("vertical movement remains bounded on empty and boundary lines") {
	TestScreen screen;
	TestFrame frame;
	TempDir dir;
	dir.write("abc\n\nxy");
	Editor::View view(dir.file());
	view.process(frame, KEY_END);
	view.process(frame, KEY_DOWN);
	view.process(frame, KEY_DOWN);
	view.process(frame, '|');
	view.process(frame, KEY_DOWN);
	view.process(frame, '|');
	view.process(frame, KEY_UP);
	view.process(frame, KEY_UP);
	view.process(frame, KEY_UP);
	view.process(frame, '|');
	REQUIRE(view.save(frame) == Editor::View::SaveResult::Saved);
	CHECK(dir.read() == "|abc\n\n|xy|");
}

TEST_CASE("space indentation reaches the next screen column after Unicode") {
	TestLocale locale;
	TestScreen screen;
	TestFrame frame;
	TempDir dir;
	dir.write("root = true\n[*]\nindent_style = space\nindent_size = 4\n", ".editorconfig");
	const std::string text = "\xe7\x95\x8c" "e\xcc\x81";
	dir.write(text);
	Editor::View view(dir.file());
	view.process(frame, KEY_END);
	view.process(frame, Control::Tab);
	view.process(frame, 'x');
	REQUIRE(view.save(frame) == Editor::View::SaveResult::Saved);
	CHECK(dir.read() == text + " x");
}

TEST_CASE("vertical navigation handles long lines in both directions") {
	TestScreen screen;
	TestFrame frame;
	TempDir dir;
	const std::string text(16384, 'a');
	dir.write("x\n" + text + "\nx");
	Editor::View view(dir.file());
	view.process(frame, KEY_DOWN);
	view.process(frame, '|');
	view.process(frame, KEY_DOWN);
	view.process(frame, KEY_UP);
	view.process(frame, '|');
	REQUIRE(view.save(frame) == Editor::View::SaveResult::Saved);
	CHECK(dir.read() == "x\n||" + text + "\nx");
}
