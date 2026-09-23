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
#include <sys/resource.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#ifdef __linux__
#include <sys/xattr.h>
#endif

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

TEST_CASE("loading and saving preserves file bytes") {
	TempDir dir;
	const std::string samples[] = {
		"", "plain", "plain\n", "\n", "\r\n", "one\r\ntwo\r\n",
		"one\r\ntwo\nthree", "one\rtwo\r", std::string("a\0b\n", 4)
	};
	for (const auto &text: samples) {
		CAPTURE(text);
		dir.write(text);
		Editor::Document doc(dir.file());
		doc.Write(dir.file());
		CHECK(dir.read() == text);
		CHECK_FALSE(doc.modified());
	}
}

TEST_CASE("editing preserves existing line endings") {
	TempDir dir;
	dir.write("a\r\nb\nc\r\n");
	Editor::Document doc(dir.file());
	doc.erase(Editor::Range({0, 1}, {1, 0}));
	doc.Write(dir.file());
	CHECK(dir.read() == "ab\nc\r\n");
	doc.split({0, 1});
	doc.Write(dir.file());
	CHECK(dir.read() == "a\r\nb\nc\r\n");
}

TEST_CASE("the final newline can be inserted and removed") {
	TempDir dir;
	dir.write("text");
	Editor::Document doc(dir.file());
	doc.split(doc.end());
	doc.Write(dir.file());
	CHECK(dir.read() == "text\n");
	doc.erase(Editor::Range(doc.end(0), doc.home(1)));
	doc.Write(dir.file());
	CHECK(dir.read() == "text");
}

TEST_CASE("failed writes preserve the original file and modified buffer") {
	TempDir dir;
	const std::string original(100, 'a');
	dir.write(original);
	pid_t child = fork();
	REQUIRE(child >= 0);
	if (child == 0) {
		Editor::Document doc(dir.file());
		doc.insert(doc.home(), 'b');
		signal(SIGXFSZ, SIG_IGN);
		struct rlimit limit = {8, 8};
		if (setrlimit(RLIMIT_FSIZE, &limit)) _exit(2);
		try {
			doc.Write(dir.file());
		} catch (const std::runtime_error &) {
			_exit(doc.modified()? 0: 3);
		}
		_exit(1);
	}
	int status;
	REQUIRE(waitpid(child, &status, 0) == child);
	REQUIRE(WIFEXITED(status));
	CHECK(WEXITSTATUS(status) == 0);
	CHECK(dir.read() == original);
	DIR *entries = opendir(dir.path.c_str());
	REQUIRE(entries != nullptr);
	while (auto entry = readdir(entries)) {
		CHECK(std::string(entry->d_name).find(".ozette-") != 0);
	}
	closedir(entries);
}

TEST_CASE("saving preserves permissions and ownership") {
	TempDir dir;
	dir.write("script\n");
	REQUIRE(chmod(dir.file().c_str(), 0751) == 0);
	struct stat before, after;
	REQUIRE(stat(dir.file().c_str(), &before) == 0);
	Editor::Document doc(dir.file());
	doc.insert(doc.home(), '#');
	doc.Write(dir.file());
	REQUIRE(stat(dir.file().c_str(), &after) == 0);
	CHECK((after.st_mode & 07777) == 0751);
	CHECK(after.st_uid == before.st_uid);
	CHECK(after.st_gid == before.st_gid);
	CHECK(after.st_ino != before.st_ino);
	CHECK(dir.read() == "#script\n");
	CHECK_FALSE(doc.modified());
}

TEST_CASE("new files respect the process umask") {
	TempDir dir;
	Editor::Document doc(dir.file());
	doc.insert(doc.home(), 'x');
	mode_t previous = umask(0027);
	try { doc.Write(dir.file()); }
	catch (...) { umask(previous); throw; }
	umask(previous);
	struct stat info;
	REQUIRE(stat(dir.file().c_str(), &info) == 0);
	CHECK((info.st_mode & 0777) == 0640);
}

TEST_CASE("saving a new empty file clears its new status") {
	TempDir dir;
	Editor::Document doc(dir.file());
	doc.Write(dir.file());
	CHECK(dir.read().empty());
	CHECK(doc.status().empty());
	CHECK_FALSE(doc.modified());
}

TEST_CASE("saving through a symlink preserves the link") {
	TempDir dir;
	dir.write("old");
	REQUIRE(symlink("file", dir.file("link").c_str()) == 0);
	Editor::Document doc(dir.file("link"));
	doc.insert(doc.home(), 'x');
	doc.Write(dir.file("link"));
	struct stat info;
	REQUIRE(lstat(dir.file("link").c_str(), &info) == 0);
	CHECK(S_ISLNK(info.st_mode));
	CHECK(dir.read() == "xold");
	CHECK(dir.read("link") == "xold");
}

TEST_CASE("saving refuses to break hard links") {
	TempDir dir;
	dir.write("old");
	REQUIRE(link(dir.file().c_str(), dir.file("alias").c_str()) == 0);
	Editor::Document doc(dir.file());
	doc.insert(doc.home(), 'x');
	CHECK_THROWS_WITH_AS(doc.Write(dir.file()),
		doctest::Contains("multiple hard links"), std::runtime_error);
	CHECK(dir.read() == "old");
	CHECK(dir.read("alias") == "old");
	CHECK(doc.modified());
	doc.Write(dir.file("copy"));
	CHECK(dir.read("copy") == "xold");
}

TEST_CASE("failed saves leave the buffer modified") {
	if (geteuid() == 0) return;
	TempDir dir;
	dir.write("old");
	Editor::Document doc(dir.file());
	doc.insert(doc.home(), 'x');
	REQUIRE(chmod(dir.file().c_str(), 0400) == 0);
	CHECK_THROWS_AS(doc.Write(dir.file()), std::runtime_error);
	CHECK(doc.modified());
	CHECK(dir.read() == "old");
}

#ifdef __linux__
TEST_CASE("saving preserves extended attributes") {
	TempDir dir;
	dir.write("old");
	REQUIRE(setxattr(dir.file().c_str(), "user.ozette-test", "value", 5, 0) == 0);
	Editor::Document doc(dir.file());
	doc.insert(doc.home(), 'x');
	doc.Write(dir.file());
	char value[16];
	ssize_t size = getxattr(dir.file().c_str(), "user.ozette-test", value, sizeof(value));
	REQUIRE(size == 5);
	CHECK(std::string(value, size) == "value");
}
#endif

TEST_CASE("external edits require explicit overwrite") {
	TempDir dir;
	dir.write("old");
	Editor::Document doc(dir.file());
	doc.insert(doc.home(), 'x');
	dir.write("external");
	CHECK_THROWS_AS(doc.Write(dir.file()), Editor::File::Changed);
	CHECK(dir.read() == "external");
	CHECK(doc.line(0) == "xold");
	CHECK(doc.modified());
	doc.Write(dir.file(), true);
	CHECK(dir.read() == "xold");
	CHECK_FALSE(doc.modified());
	doc.insert(doc.home(), 'y');
	CHECK_NOTHROW(doc.Write(dir.file()));
	CHECK(dir.read() == "yxold");
}

TEST_CASE("external replacement and removal are detected") {
	TempDir dir;
	dir.write("old");
	Editor::Document doc(dir.file());
	doc.insert(doc.home(), 'x');
	SUBCASE("replacement with the same size and modification time") {
		struct stat info;
		REQUIRE(stat(dir.file().c_str(), &info) == 0);
		dir.write("new", "replacement");
#ifdef __APPLE__
		struct timespec times[] = {info.st_atimespec, info.st_mtimespec};
#else
		struct timespec times[] = {info.st_atim, info.st_mtim};
#endif
		REQUIRE(utimensat(AT_FDCWD, dir.file("replacement").c_str(), times, 0) == 0);
		REQUIRE(rename(dir.file("replacement").c_str(), dir.file().c_str()) == 0);
		CHECK_THROWS_AS(doc.Write(dir.file()), Editor::File::Changed);
		CHECK(dir.read() == "new");
	}
	SUBCASE("removal") {
		REQUIRE(unlink(dir.file().c_str()) == 0);
		CHECK_THROWS_AS(doc.Write(dir.file()), Editor::File::Changed);
		CHECK(access(dir.file().c_str(), F_OK) != 0);
		doc.Write(dir.file(), true);
		CHECK(dir.read() == "xold");
	}
}

TEST_CASE("new and Save As destinations do not overwrite existing files silently") {
	TempDir dir;
	Editor::Document doc(dir.file());
	doc.insert(doc.home(), 'x');
	dir.write("created elsewhere");
	CHECK_THROWS_AS(doc.Write(dir.file()), Editor::File::Changed);
	CHECK(dir.read() == "created elsewhere");
	dir.write("another file", "other");
	CHECK_THROWS_AS(doc.Write(dir.file("other")), Editor::File::Changed);
	CHECK(dir.read("other") == "another file");
	doc.Write(dir.file("other"), true);
	CHECK(dir.read("other") == "x");
}

TEST_CASE("retargeted symlinks require confirmation") {
	TempDir dir;
	dir.write("old");
	dir.write("other", "other");
	REQUIRE(symlink("file", dir.file("link").c_str()) == 0);
	Editor::Document doc(dir.file("link"));
	doc.insert(doc.home(), 'x');
	REQUIRE(unlink(dir.file("link").c_str()) == 0);
	REQUIRE(symlink("other", dir.file("link").c_str()) == 0);
	CHECK_THROWS_AS(doc.Write(dir.file("link")), Editor::File::Changed);
	CHECK(dir.read() == "old");
	CHECK(dir.read("other") == "other");
}
