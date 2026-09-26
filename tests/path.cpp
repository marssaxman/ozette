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
#include "app/path.h"
#include "files.h"
#include <sys/stat.h>

TEST_CASE("absolute paths preserve root and normalize ordinary aliases") {
	CHECK(Path::absolute("/") == "/");
	CHECK(Path::absolute("/.././") == "/");
	CHECK(Path::absolute(".") == Path::current_dir());
	CHECK(Path::absolute("./file") == Path::current_dir() + "/file");
	CHECK(Path::absolute("~/file") == Path::home_dir() + "/file");
	TempDir dir;
	CHECK(Path::absolute(dir.path + "//./file") == dir.file());
}

TEST_CASE("parent references follow directory symlinks") {
	TempDir dir, elsewhere;
	REQUIRE(mkdir(elsewhere.file("sub").c_str(), 0700) == 0);
	REQUIRE(symlink(elsewhere.file("sub").c_str(), dir.file("link").c_str()) == 0);
	CHECK(Path::absolute(dir.file("link/../file")) == elsewhere.file());
	CHECK(Path::absolute(dir.file("link/file")) == dir.file("link/file"));
	CHECK(Path::absolute(dir.file("missing/../file")) == dir.file("missing/../file"));
}

TEST_CASE("file identity recognizes links and missing targets") {
	TempDir dir;
	dir.write("old");
	REQUIRE(symlink("file", dir.file("symbolic").c_str()) == 0);
	REQUIRE(link(dir.file().c_str(), dir.file("hard").c_str()) == 0);
	REQUIRE(symlink(".", dir.file("directory").c_str()) == 0);
	CHECK(Path::same_file(dir.file(), dir.file("./file")));
	CHECK(Path::same_file(dir.file(), dir.file("symbolic")));
	CHECK(Path::same_file(dir.file(), dir.file("hard")));
	CHECK(Path::same_file(dir.file("new"), dir.file("directory/new")));
	CHECK(Path::same_file(dir.file("missing/new"), dir.file("directory/missing/new")));
	CHECK_FALSE(Path::same_file(dir.file(), dir.file("new")));
	CHECK_FALSE(Path::same_file(dir.file("new"), dir.file("other")));
}

TEST_CASE("file identity follows replacements instead of caching inodes") {
	TempDir dir;
	dir.write("old");
	REQUIRE(link(dir.file().c_str(), dir.file("alias").c_str()) == 0);
	CHECK(Path::same_file(dir.file(), dir.file("alias")));
	dir.write("new", "replacement");
	REQUIRE(rename(dir.file("replacement").c_str(), dir.file().c_str()) == 0);
	CHECK_FALSE(Path::same_file(dir.file(), dir.file("alias")));
	REQUIRE(unlink(dir.file("alias").c_str()) == 0);
	REQUIRE(link(dir.file().c_str(), dir.file("alias").c_str()) == 0);
	CHECK(Path::same_file(dir.file(), dir.file("alias")));
}

TEST_CASE("display paths abbreviate complete directory components") {
	std::string cwd = Path::current_dir();
	CHECK(Path::display(cwd + "/file") == "file");
	CHECK(Path::display(cwd + "-other/file") != "other/file");
	CHECK(Path::display(Path::home_dir() + "-other/file") != "~-other/file");
}
