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
	CHECK(frame.controller.renamed.empty());
	CHECK(dir.read("other") == "other");
	view.process(frame, Control::SaveAs);
	frame.enter(dir.file("other"));
	frame.answer('y');
	CHECK_FALSE(view.is_modified());
	CHECK(frame.controller.renamed == dir.file("other"));
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
	CHECK(frame.controller.closed.empty());
	frame.answer(Control::Escape);
	CHECK(frame.controller.closed.empty());
	CHECK(view.is_modified());
	view.process(frame, Control::Close);
	frame.answer('y');
	frame.answer('y');
	CHECK(frame.controller.closed == dir.file());
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
