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
#include <sys/stat.h>

TEST_CASE("missing and empty files have a valid empty document") {
	TempDir dir;
	Editor::Document missing(dir.file());
	CHECK(missing.status() == "New");
	CHECK((missing.end() == missing.home()));
	CHECK_FALSE(missing.modified());
	dir.write("");
	Editor::Document empty(dir.file());
	CHECK(empty.status().empty());
	CHECK((empty.end() == empty.home()));
	CHECK_FALSE(empty.modified());
}

TEST_CASE("reading blank lines is safe") {
	TempDir dir;
	dir.write("\ntext\n\n");
	Editor::Document doc(dir.file());
	CHECK(doc.line(0).empty());
	CHECK(doc.line(1) == "text");
	CHECK(doc.line(2).empty());
}

TEST_CASE("unsupported files and invalid paths report read errors") {
	TempDir dir;
	CHECK_THROWS_WITH_AS(Editor::Document(dir.path),
		doctest::Contains("Not a regular file"), std::runtime_error);
	REQUIRE(mkfifo(dir.file("pipe").c_str(), 0600) == 0);
	CHECK_THROWS_WITH_AS(Editor::Document(dir.file("pipe")),
		doctest::Contains("Not a regular file"), std::runtime_error);
	dir.write("text");
	CHECK_THROWS_WITH_AS(Editor::Document(dir.file() + "/child"),
		doctest::Contains("Can't read"), std::runtime_error);
	REQUIRE(symlink("missing", dir.file("link").c_str()) == 0);
	CHECK_THROWS_WITH_AS(Editor::Document(dir.file("link")),
		doctest::Contains("Missing link target"), std::runtime_error);
}

TEST_CASE("unreadable files are not treated as new files") {
	if (geteuid() == 0) return;
	TempDir dir;
	dir.write("private");
	REQUIRE(chmod(dir.file().c_str(), 0000) == 0);
	CHECK_THROWS_WITH_AS(Editor::Document(dir.file()),
		doctest::Contains("Can't read"), std::runtime_error);
}

#ifdef __linux__
TEST_CASE("read failures are not treated as empty files") {
	CHECK_THROWS_WITH_AS(Editor::Document("/proc/self/mem"),
		doctest::Contains("Can't read"), std::runtime_error);
}
#endif
