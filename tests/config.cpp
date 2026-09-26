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
#include "editor/config.h"
#include "files.h"
#include <sys/stat.h>

namespace {
void settings(const TempDir &dir, const std::string &text) {
	dir.write("root = true\n[*]\n" + text, ".editorconfig");
}
} // namespace

TEST_CASE("configuration defaults remain usable without settings") {
	TempDir dir;
	dir.write("root = true\n", ".editorconfig");
	Editor::Config config;
	CHECK(config.indent_style() == '\t');
	CHECK(config.indent_size() == 4);
	config.load(dir.file());
	CHECK(config.indent_style() == '\t');
	CHECK(config.indent_size() == 4);
}

TEST_CASE("symbolic indentation is resolved after all matching settings") {
	TempDir dir;
	Editor::Config config;
	for (const auto &text: {
		"indent_style = space\nindent_size = tab\ntab_width = 3\n",
		"tab_width = 3\nindent_size = TAB\nindent_style = space\n",
		"indent_size = tab\n[fi*]\ntab_width = 3\nindent_style = space\n"}) {
		settings(dir, text);
		config.load(dir.file());
		CHECK(config.indent_size() == 3);
		CHECK(config.indent_style() == ' ');
	}
	settings(dir, "indent_size = tab\n");
	config.load(dir.file());
	CHECK(config.indent_size() == 4);
	settings(dir, "tab_width = 6\n");
	config.load(dir.file());
	CHECK(config.indent_size() == 6);
	settings(dir, "indent_size = 2\ntab_width = 6\n");
	config.load(dir.file());
	CHECK(config.indent_size() == 2);
}

TEST_CASE("numeric indentation accepts only bounded positive decimal values") {
	TempDir dir;
	Editor::Config config;
	const std::string invalid[] = {"", "0", "000", "-1", "+2", "2px", "2.5", "2 4",
		"0x10", "257", "4294967296", "18446744073709551616", std::string(4096, '9'),
		"garbage", "4 # comment", "4 ; comment", "\xff"};
	for (const auto &value: invalid) {
		CAPTURE(value);
		settings(dir, "indent_size = " + value + "\n");
		CHECK_NOTHROW(config.load(dir.file()));
		CHECK(config.indent_size() == 4);
		settings(dir, "indent_size = tab\ntab_width = " + value + "\n");
		CHECK_NOTHROW(config.load(dir.file()));
		CHECK(config.indent_size() == 4);
	}
	for (unsigned size: {1U, 2U, 8U, 256U}) {
		settings(dir, "indent_size = " + std::to_string(size) + "\n");
		config.load(dir.file());
		CHECK(config.indent_size() == size);
	}
	settings(dir, "indent_size = 0003\n");
	config.load(dir.file());
	CHECK(config.indent_size() == 3);
}

TEST_CASE("unset removes inherited properties while invalid values are ignored") {
	TempDir dir;
	Editor::Config config;
	for (const auto &key: {"indent_size", "indent_style", "tab_width"}) {
		CAPTURE(key);
		settings(dir, "indent_style = space\nindent_size = 2\ntab_width = 7\n"
			"[file]\n" + std::string(key) + " = unset\n");
		config.load(dir.file());
		CHECK(config.indent_style() == (std::string(key) == "indent_style"? '\t': ' '));
		CHECK(config.indent_size() == (std::string(key) == "indent_size"? 4: 2));
	}
	settings(dir, "indent_style = space\nindent_size = 3\n[file]\n"
		"indent_size = 0\nindent_style = unknown\n");
	config.load(dir.file());
	CHECK(config.indent_size() == 3);
	CHECK(config.indent_style() == ' ');
	settings(dir, "indent_size = tab\ntab_width = 7\n[file]\ntab_width = unset\n");
	config.load(dir.file());
	CHECK(config.indent_size() == 4);
}

TEST_CASE("parent configuration resolves symbolic values with child overrides") {
	TempDir dir;
	REQUIRE(mkdir(dir.file("sub").c_str(), 0700) == 0);
	settings(dir, "indent_style = space\nindent_size = tab\ntab_width = 2\n");
	dir.write("[*]\ntab_width = 5\n", "sub/.editorconfig");
	Editor::Config config;
	config.load(dir.file("sub/file"));
	CHECK(config.indent_style() == ' ');
	CHECK(config.indent_size() == 5);
	dir.write("ROOT = TRUE  \r\n[*]\ntab_width = 6\n", "sub/.editorconfig");
	config.load(dir.file("sub/file"));
	CHECK(config.indent_style() == '\t');
	CHECK(config.indent_size() == 6);
	REQUIRE(std::remove(dir.file("sub/.editorconfig").c_str()) == 0);
}

TEST_CASE("configuration trims whitespace and keeps filename glob case") {
	TempDir dir;
	dir.write(" ROOT = TRUE \r\n [Fi[ln]e] \r\n INDENT_STYLE = SPACE \r\n"
		" INDENT_SIZE = TAB \r\n TAB_WIDTH = 3 \r\n", ".editorconfig");
	Editor::Config config;
	config.load(dir.file("File"));
	CHECK(config.indent_style() == ' ');
	CHECK(config.indent_size() == 3);
	config.load(dir.file("file"));
	CHECK(config.indent_style() == '\t');
	CHECK(config.indent_size() == 4);
	settings(dir, "indent_size = 2\n[/sub/*.cpp]\nindent_size = 6\n");
	config.load(dir.file("sub/code.cpp"));
	CHECK(config.indent_size() == 6);
	config.load(dir.file("sub/code.h"));
	CHECK(config.indent_size() == 2);
}

TEST_CASE("malformed unreadable and unsupported configuration remains recoverable") {
	TempDir dir;
	Editor::Config config;
	for (const auto &text: {"indent_size = 3\n[broken\n", "indent_size = 3\n= value\n",
		"indent_size = 3\nmissing separator\n"}) {
		settings(dir, text);
		CHECK_NOTHROW(config.load(dir.file()));
		CHECK(config.indent_size() == 4);
	}
	settings(dir, "indent_size = 2\nmax_line_length = off\ncharset = nonsense\n"
		"end_of_line = crlf\ninsert_final_newline = true\ntrim_trailing_whitespace = true\n");
	CHECK_NOTHROW(config.load(dir.file()));
	CHECK(config.indent_size() == 2);
	if (geteuid() != 0) {
		REQUIRE(chmod(dir.file(".editorconfig").c_str(), 0000) == 0);
		CHECK_NOTHROW(config.load(dir.file()));
		CHECK(config.indent_size() == 4);
		REQUIRE(chmod(dir.file(".editorconfig").c_str(), 0600) == 0);
	}
}

TEST_CASE("malformed glob escapes cannot run past the configuration pattern") {
	TempDir dir;
	Editor::Config config;
	for (const auto &pattern: {"{a,b\\}", "{a\\,b\\}", "[abc", "{a,b", "\\"}) {
		settings(dir, "indent_size = 2\n[" + std::string(pattern) + "]\nindent_size = 7\n");
		CHECK_NOTHROW(config.load(dir.file()));
		CHECK(config.indent_size() == 2);
	}
	settings(dir, "indent_size = 2\n[/*]\nindent_size = 7\n");
	config.load(dir.file());
	CHECK(config.indent_size() == 7);
	config.load(dir.file("sub/file"));
	CHECK(config.indent_size() == 2);
}
