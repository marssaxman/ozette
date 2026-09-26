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
#include <sys/stat.h>

using SaveResult = Editor::View::SaveResult;

TEST_CASE("conflicting saves can be cancelled or explicitly overwritten") {
	TestScreen screen;
	TestFrame frame;
	TempDir dir;
	dir.write("old");
	Editor::View view(dir.file());
	view.process(frame, 'x');
	dir.write("external");
	CHECK(view.save(frame) == SaveResult::Pending);
	frame.answer(Control::Escape);
	CHECK(view.is_modified());
	CHECK(dir.read() == "external");
	CHECK(view.save(frame) == SaveResult::Pending);
	frame.answer('y');
	CHECK_FALSE(view.is_modified());
	CHECK(dir.read() == "xold");
	CHECK(view.save(frame) == SaveResult::Saved);
}

TEST_CASE("reloading requires confirmation before discarding edits") {
	TestScreen screen;
	TestFrame frame;
	TempDir dir;
	dir.write("old");
	Editor::View view(dir.file());
	view.process(frame, 'x');
	dir.write("external\r\n");
	REQUIRE(view.save(frame) == SaveResult::Pending);
	frame.answer('n');
	frame.answer(Control::Escape);
	CHECK(view.is_modified());
	REQUIRE(view.save(frame) == SaveResult::Pending);
	frame.answer('n');
	frame.answer('y');
	CHECK_FALSE(view.is_modified());
	view.process(frame, 'y');
	CHECK(view.save(frame) == SaveResult::Saved);
	CHECK(dir.read() == "yexternal\r\n");
}

TEST_CASE("a failed reload preserves unsaved edits") {
	if (geteuid() == 0) return;
	TestScreen screen;
	TestFrame frame;
	TempDir dir;
	dir.write("old");
	Editor::View view(dir.file());
	view.process(frame, 'x');
	dir.write("external");
	REQUIRE(view.save(frame) == SaveResult::Pending);
	frame.answer('n');
	REQUIRE(chmod(dir.file().c_str(), 0000) == 0);
	frame.answer('y');
	CHECK(view.is_modified());
	CHECK(frame.result.find("Can't read") != std::string::npos);
	REQUIRE(chmod(dir.file().c_str(), 0600) == 0);
	REQUIRE(view.save(frame) == SaveResult::Pending);
	frame.answer('y');
	CHECK(dir.read() == "xold");
}

TEST_CASE("Save As changes the editor target only after a successful write") {
	TestScreen screen;
	TestFrame frame;
	TempDir dir;
	dir.write("old");
	dir.write("other", "other");
	Editor::View view(dir.file());
	view.process(frame, 'x');
	view.process(frame, Control::SaveAs);
	frame.enter(dir.file("other"));
	frame.answer('n');
	CHECK(view.is_modified());
	CHECK(view.target_path() == dir.file());
	CHECK(dir.read("other") == "other");
	view.process(frame, Control::SaveAs);
	frame.enter(dir.file("other"));
	frame.answer('y');
	CHECK_FALSE(view.is_modified());
	CHECK(view.target_path() == dir.file("other"));
	CHECK(dir.read("other") == "xold");
	CHECK(dir.read() == "old");
	view.process(frame, 'y');
	CHECK(view.save(frame) == SaveResult::Saved);
	CHECK(dir.read("other") == "xyold");
}

TEST_CASE("closing waits for a successful save after resolving a conflict") {
	TestScreen screen;
	TestFrame frame;
	TempDir dir;
	dir.write("old");
	Editor::View view(dir.file());
	view.process(frame, 'x');
	dir.write("external");
	view.process(frame, Control::Close);
	frame.answer('y');
	CHECK(frame.controller.closed == nullptr);
	frame.answer(Control::Escape);
	CHECK(frame.controller.closed == nullptr);
	CHECK(view.is_modified());
	view.process(frame, Control::Close);
	frame.answer('y');
	frame.answer('y');
	CHECK(frame.controller.closed == &view);
	CHECK(dir.read() == "xold");
}

TEST_CASE("save results distinguish failure and unfinished Save As") {
	TestScreen screen;
	TestFrame frame;
	TempDir dir;
	SUBCASE("hard link refusal") {
		dir.write("old");
		REQUIRE(link(dir.file().c_str(), dir.file("link").c_str()) == 0);
		Editor::View view(dir.file());
		view.process(frame, 'x');
		CHECK(view.save(frame) == SaveResult::Failed);
		CHECK(view.is_modified());
		CHECK(dir.read() == "old");
	}
	SUBCASE("untitled buffer") {
		Editor::View view;
		view.process(frame, 'x');
		CHECK(view.save(frame) == SaveResult::Pending);
		frame.answer(Control::Escape);
		CHECK(view.is_modified());
		CHECK(view.save(frame) == SaveResult::Pending);
		frame.enter(dir.file());
		CHECK_FALSE(view.is_modified());
		CHECK(dir.read() == "x");
	}
}

namespace {
void check_text(Editor::View &view, TestFrame &frame, const std::string &expected) {
	// Select using navigation, so unexpectedly short text cannot create an invalid
	// cursor. Extra lines or bytes must also appear in the copied text.
	view.select(frame, Editor::Range({0, 0}, {0, 0}));
	for (size_t i = 0; i < expected.size() + 2; ++i) view.process(frame, KEY_SF);
	view.process(frame, Control::Copy);
	CHECK(frame.controller.clipboard == expected);
}
} // namespace

TEST_CASE("typing remains grouped until the cursor moves") {
	TestScreen screen;
	TestFrame frame;
	Editor::View view;
	view.process(frame, 'a');
	view.process(frame, 'b');
	view.process(frame, KEY_LEFT);
	view.process(frame, KEY_RIGHT);
	view.process(frame, 'c');
	view.process(frame, Control::Undo);
	check_text(view, frame, "ab");
	view.process(frame, Control::Undo);
	CHECK_FALSE(view.is_modified());
	view.process(frame, Control::Redo);
	view.process(frame, Control::Redo);
	check_text(view, frame, "abc");
}

TEST_CASE("consecutive deletes and backspaces form one undo group") {
	TestScreen screen;
	TestFrame frame;
	TempDir dir;
	dir.write("abcdefgh");
	Editor::View view(dir.file());
	SUBCASE("delete") {
		view.process(frame, KEY_DC);
		view.process(frame, KEY_DC);
		check_text(view, frame, "cdefgh");
	}
	SUBCASE("backspace") {
		view.process(frame, KEY_END);
		view.process(frame, Control::Backspace);
		view.process(frame, Control::Backspace);
		check_text(view, frame, "abcdef");
	}
	view.process(frame, Control::Undo);
	CHECK_FALSE(view.is_modified());
	check_text(view, frame, "abcdefgh");
}

TEST_CASE("typing over a selection undoes as one replacement") {
	TestScreen screen;
	TestFrame frame;
	TempDir dir;
	dir.write("old text");
	Editor::View view(dir.file());
	view.select(frame, Editor::Range({0, 0}, {0, 3}));
	for (char ch: std::string("new")) view.process(frame, ch);
	view.process(frame, Control::Undo);
	CHECK_FALSE(view.is_modified());
	check_text(view, frame, "old text");
	view.process(frame, Control::Redo);
	check_text(view, frame, "new text");
}

TEST_CASE("cut and multiline paste each undo as one command") {
	TestScreen screen;
	TestFrame frame;
	TempDir dir;
	dir.write("alpha\nbeta\ngamma");
	Editor::View view(dir.file());
	view.select(frame, Editor::Range({0, 2}, {2, 2}));
	view.process(frame, Control::Cut);
	CHECK(frame.controller.clipboard == "pha\nbeta\nga");
	view.process(frame, Control::Paste);
	view.process(frame, Control::Undo);
	check_text(view, frame, "almma");
	view.process(frame, Control::Undo);
	CHECK_FALSE(view.is_modified());
	check_text(view, frame, "alpha\nbeta\ngamma");
	view.process(frame, Control::Redo);
	view.process(frame, Control::Redo);
	check_text(view, frame, "alpha\nbeta\ngamma");
}

TEST_CASE("paste does not absorb neighboring typing") {
	TestScreen screen;
	TestFrame frame;
	Editor::View view;
	view.process(frame, 'a');
	frame.controller.clipboard = "b\nc";
	view.process(frame, Control::Paste);
	view.process(frame, 'd');
	view.process(frame, Control::Undo);
	check_text(view, frame, "ab\nc");
	view.process(frame, Control::Undo);
	check_text(view, frame, "a");
	view.process(frame, Control::Undo);
	CHECK_FALSE(view.is_modified());
}

TEST_CASE("line splitting and automatic indentation undo together") {
	TestScreen screen;
	TestFrame frame;
	TempDir dir;
	dir.write("  alpha");
	Editor::View view(dir.file());
	view.process(frame, KEY_END);
	int key = Control::Return;
	SUBCASE("return moves to the new indented line") { key = Control::Return; }
	SUBCASE("enter leaves the cursor on the original line") { key = Control::Enter; }
	view.process(frame, key);
	view.process(frame, 'x');
	view.process(frame, Control::Undo);
	check_text(view, frame, key == Control::Return? "  alpha\n  ": "  alpha\n");
	view.process(frame, Control::Undo);
	CHECK_FALSE(view.is_modified());
	check_text(view, frame, "  alpha");
	view.process(frame, Control::Redo);
	view.process(frame, Control::Redo);
	check_text(view, frame, key == Control::Return? "  alpha\n  x": "  alphax\n");
}

TEST_CASE("indentation commands undo as single transactions") {
	TestScreen screen;
	TestFrame frame;
	TempDir dir;
	dir.write("root = true\n[*]\nindent_style = space\nindent_size = 2\n", ".editorconfig");
	dir.write("a\nb");
	Editor::View view(dir.file());
	SUBCASE("indent and unindent selected lines") {
		view.select(frame, Editor::Range({0, 0}, {1, 1}));
		view.process(frame, Control::Tab);
		view.process(frame, KEY_BTAB);
		view.process(frame, Control::Undo);
		check_text(view, frame, "  a\n  b");
		view.process(frame, Control::Undo);
		CHECK_FALSE(view.is_modified());
		check_text(view, frame, "a\nb");
		view.process(frame, Control::Redo);
		view.process(frame, Control::Redo);
		check_text(view, frame, "a\nb");
	}
	SUBCASE("tab inserts one indentation step between typing groups") {
		view.process(frame, 'x');
		view.process(frame, Control::Tab);
		view.process(frame, 'y');
		view.process(frame, Control::Undo);
		check_text(view, frame, "x a\nb");
		view.process(frame, Control::Undo);
		check_text(view, frame, "xa\nb");
		view.process(frame, Control::Undo);
		CHECK_FALSE(view.is_modified());
	}
}

TEST_CASE("undo and redo with no history leave the cursor in place") {
	TestScreen screen;
	TestFrame frame;
	TempDir dir;
	dir.write("abc");
	Editor::View view(dir.file());
	view.process(frame, KEY_END);
	view.process(frame, Control::Undo);
	view.process(frame, Control::Redo);
	view.process(frame, 'd');
	check_text(view, frame, "abcd");
}

TEST_CASE("successive replacements undo separately, including empty replacements") {
	TestScreen screen;
	TestFrame frame;
	TempDir dir;
	dir.write("aa aa");
	Editor::View view(dir.file());
	std::string replacement;
	SUBCASE("replace with text") { replacement = "x"; }
	SUBCASE("replace with nothing") { replacement = ""; }
	view.process(frame, Control::Replace);
	frame.answer('a');
	frame.answer('a');
	frame.answer(KEY_DOWN);
	frame.enter(replacement);
	view.process(frame, Control::FindNext);
	view.process(frame, Control::Undo);
	check_text(view, frame, replacement + " aa");
	view.process(frame, Control::Undo);
	CHECK_FALSE(view.is_modified());
	check_text(view, frame, "aa aa");
	view.process(frame, Control::Redo);
	view.process(frame, Control::Redo);
	check_text(view, frame, replacement + " " + replacement);
}

TEST_CASE("a refused save preserves the target and unsaved edits") {
	TestScreen screen;
	TestFrame frame;
	TempDir dir;
	dir.write("old");
	Editor::View view(dir.file());
	view.process(frame, 'x');
	frame.controller.allow_save = false;
	CHECK(view.save(frame) == SaveResult::Failed);
	CHECK_FALSE(frame.dialog);
	CHECK(frame.result.find("File already open") != std::string::npos);
	CHECK(view.is_modified());
	CHECK(view.target_path() == dir.file());
	CHECK(dir.read() == "old");
	view.process(frame, Control::Close);
	frame.answer('y');
	CHECK(frame.controller.closed == nullptr);
	CHECK(view.is_modified());
}
