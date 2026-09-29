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
#include "dialog/input.h"
#include "editor/editor.h"
#include "files.h"
#include "ui.h"
#include "utf8_locale.h"

namespace {
void type(Editor::View &view, TestFrame &frame, const std::string &text) {
	for (unsigned char ch: text) view.process(frame, ch);
}

void type(Dialog::Input &input, TestFrame &frame, const std::string &text) {
	for (unsigned char ch: text) input.process(frame, ch);
}
} // namespace

TEST_CASE("UTF-8 typing replaces a selection and retains ordinary undo grouping") {
	TestLocale locale;
	TestScreen screen;
	TestFrame frame;
	TempDir dir;
	dir.write("old text");
	Editor::View view(dir.file());
	const std::string text = "\xc3\xa9\xe7\x95\x8c" "e\xcc\x81\xf0\x9f\x90\xb1\xc8\xab";
	view.select(frame, Editor::Range({0, 0}, {0, 3}));
	type(view, frame, text);
	view.paint(UI::View::State::Focused);
	REQUIRE(view.save(frame) == Editor::View::SaveResult::Saved);
	CHECK(dir.read() == text + " text");
	view.process(frame, Control::Undo);
	REQUIRE(view.save(frame) == Editor::View::SaveResult::Saved);
	CHECK(dir.read() == "old text");
	view.process(frame, Control::Redo);
	view.process(frame, Control::Backspace);
	REQUIRE(view.save(frame) == Editor::View::SaveResult::Saved);
	CHECK(dir.read() == text.substr(0, text.size() - 2) + " text");
}

TEST_CASE("partial and malformed input bytes remain editable without loss") {
	TestLocale locale;
	TestScreen screen;
	TestFrame frame;
	TempDir dir;
	dir.write("");
	Editor::View view(dir.file());
	const std::string bytes = "\x80\xf0\x9f\xe2\x82";
	for (unsigned char ch: bytes) {
		view.process(frame, ch);
		view.paint(UI::View::State::Focused);
	}
	REQUIRE(view.save(frame) == Editor::View::SaveResult::Saved);
	CHECK(dir.read() == bytes);
	view.process(frame, Control::Backspace);
	view.process(frame, Control::Undo);
	REQUIRE(view.save(frame) == Editor::View::SaveResult::Saved);
	CHECK(dir.read() == bytes);
}

TEST_CASE("Unicode search fields find and replace multibyte text") {
	TestLocale locale;
	TestScreen screen;
	TestFrame frame;
	TempDir dir;
	dir.write("before \xe7\x95\x8c after");
	Editor::View view(dir.file());
	view.process(frame, Control::Find);
	frame.enter("\xe7\x95\x8c");
	view.process(frame, '|');
	REQUIRE(view.save(frame) == Editor::View::SaveResult::Saved);
	CHECK(dir.read() == "before | after");
}

TEST_CASE("dialog navigation and deletion stop at UTF-8 boundaries") {
	TestLocale locale;
	TestFrame frame;
	const std::string text = "\xc3\xa9\xe7\x95\x8c" "e\xcc\x81\xf0\x9f\x90\xb1";
	Dialog::Input input(text, nullptr, nullptr);
	input.process(frame, KEY_RIGHT);
	input.process(frame, Control::Backspace);
	CHECK(input.value() == text.substr(0, text.size() - 4));
	input.process(frame, Control::Backspace);
	CHECK(input.value() == "\xc3\xa9\xe7\x95\x8c" "e");
	input.process(frame, KEY_LEFT);
	input.process(frame, KEY_LEFT);
	input.process(frame, KEY_DC);
	CHECK(input.value() == "\xc3\xa9" "e");
	input.process(frame, KEY_SLEFT);
	input.process(frame, Control::Copy);
	CHECK(frame.controller.clipboard == "\xc3\xa9");
	input.process(frame, Control::Paste);
	CHECK(input.value() == "\xc3\xa9" "e");
}

TEST_CASE("dialog typing retains non-ASCII bytes and separates special keys") {
	TestFrame frame;
	Dialog::Input input("", nullptr, nullptr);
	const std::string text = "\xc3\xa9\xc8\xab\x80\xe2\x82";
	type(input, frame, text);
	input.process(frame, KEY_UP);
	input.process(frame, -1);
	CHECK(input.value() == text);
	input.process(frame, Control::Backspace);
	CHECK(input.value() == text.substr(0, text.size() - 1));
}

TEST_CASE("dialog painting shares wide character columns and clips its selection") {
	TestLocale locale;
	TestScreen screen;
	TestFrame frame;
	Dialog::Input input("A\xe7\x95\x8c" "e\xcc\x81Z", nullptr, nullptr);
	wattrset(stdscr, A_REVERSE);
	input.paint(stdscr, 0, 2, 8, UI::View::State::Focused);
	CHECK_FALSE((mvwinch(stdscr, 0, 2) & A_REVERSE) != 0);
	CHECK_FALSE((mvwinch(stdscr, 0, 6) & A_REVERSE) != 0);
	CHECK((mvwinch(stdscr, 0, 7) & A_REVERSE) != 0);
	input.process(frame, KEY_RIGHT);
	input.paint(stdscr, 0, 2, 8, UI::View::State::Focused);
	int row, column;
	getyx(stdscr, row, column);
	CHECK(row == 0);
	CHECK(column == 7);
	input.paint(stdscr, 0, 2, 3, UI::View::State::Focused);
	getyx(stdscr, row, column);
	CHECK(column == 4);
	CHECK((mvwinch(stdscr, 0, 3) & A_CHARTEXT) == 'Z');
	input.paint(stdscr, 0, 2, 0, UI::View::State::Focused);
	CHECK(input.value() == "A\xe7\x95\x8c" "e\xcc\x81Z");
}
