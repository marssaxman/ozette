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
#include "search/result.h"
#include "search/search.h"
#include "app/path.h"
#include "files.h"
#include "ui.h"
#include <chrono>
#include <limits>
#include <sys/stat.h>
#include <thread>

namespace {
class TestSearch : public Search::View {
public:
	static Search::View &current() { return *_instance; }
};

struct WorkingDirectory {
	WorkingDirectory(std::string path): previous(Path::current_dir()) {
		if (chdir(path.c_str()) < 0) throw std::runtime_error("chdir failed");
	}
	~WorkingDirectory() {
		if (chdir(previous.c_str()) < 0) std::terminate();
	}
	std::string previous;
};

struct FakeGrep {
	FakeGrep(const TempDir &dir, std::string script): previous(getenv("PATH")) {
		dir.write("#!/bin/sh\n" + script, "grep");
		if (chmod(dir.file("grep").c_str(), 0755) < 0 ||
			setenv("PATH", (dir.path + ":" + previous).c_str(), 1) < 0) {
			throw std::runtime_error("fake grep setup failed");
		}
	}
	~FakeGrep() { setenv("PATH", previous.c_str(), 1); }
	std::string previous;
};

void finish(Search::View &view, TestFrame &frame) {
	auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
	do {
		view.poll(frame);
		if (frame.status != "running" && frame.status != "cancelling") return;
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	} while (std::chrono::steady_clock::now() < deadline);
	FAIL("search did not finish");
}

std::string displayed(Search::View &view) {
	view.layout(0, 0, 12, 80);
	view.bring_forward();
	view.paint(UI::View::State::Focused);
	WINDOW *window = panel_window(panel_below(nullptr));
	REQUIRE(window);
	std::string text;
	for (int row = 0; row < 12; ++row) {
		char line[81] = {};
		REQUIRE(mvwinnstr(window, row, 0, line, 80) != ERR);
		text += line;
	}
	return text;
}

std::string record(std::string path, std::string number, std::string text = "match") {
	return path + '\0' + number + ":" + text + "\n";
}
} // namespace

TEST_CASE("search parser preserves filenames and text across record boundaries") {
	Search::Parser parser;
	Search::Match match;
	std::string path = "foo:bar\n\t\xc3\xa9";
	unsigned matches = 0;
	for (char ch: record(path, "12", "text:with:colons\t\xc3\xa9")) {
		if (parser.read(ch, match)) ++matches;
	}
	parser.finish();
	CHECK_FALSE(parser.failed());
	CHECK(matches == 1);
	CHECK(match.path == path);
	CHECK(match.index == 11);
	CHECK(match.text == "text:with:colons\t\xc3\xa9");
}

TEST_CASE("search parser rejects malformed line numbers and recovers for later records") {
	for (std::string number: {"", "0", "-1", "+1", "1x", " 1", "184467440737095516160000"}) {
		CAPTURE(number);
		Search::Parser parser;
		Search::Match match;
		unsigned matches = 0;
		for (char ch: record("bad", number) + record("good", "2")) {
			if (parser.read(ch, match)) ++matches;
		}
		parser.finish();
		CHECK(parser.failed());
		CHECK(matches == 1);
		CHECK(match.path == "good");
		CHECK(match.index == 1);
	}
}

TEST_CASE("search parser validates empty paths and incomplete records") {
	for (const auto &text: {record("", "1"), std::string("file"),
		std::string("file\0", 5), std::string("file\0001", 6), std::string("file\0001:match", 12)}) {
		Search::Parser parser;
		Search::Match match;
		for (char ch: text) CHECK_FALSE(parser.read(ch, match));
		parser.finish();
		CHECK(parser.failed());
	}
	Search::Parser empty;
	empty.finish();
	CHECK_FALSE(empty.failed());
}

TEST_CASE("search parser accepts the largest representable line number") {
	Search::Parser parser;
	Search::Match match;
	bool found = false;
	for (char ch: record("file", std::to_string(std::numeric_limits<size_t>::max()))) {
		found |= parser.read(ch, match);
	}
	CHECK(found);
	CHECK(match.index == std::numeric_limits<size_t>::max() - 1);
	CHECK_FALSE(parser.failed());
}

TEST_CASE("external search opens filenames containing delimiters at the matching line") {
	TestScreen screen;
	TestFrame frame;
	UI::Shell shell(frame.controller);
	TempDir dir;
	for (std::string name: {"foo:bar", "line\nbreak", "tab\tname", "utf8-\xc3\xa9"}) {
		dir.write("first\nneedle:with:colons\n", name);
		Search::View::exec({"needle", dir.file(name), ""}, shell);
		auto &view = TestSearch::current();
		finish(view, frame);
		CHECK(frame.status == "1 matches in 1 files (exit 0)");
		view.process(frame, KEY_DOWN);
		view.process(frame, Control::Return);
		CHECK(frame.controller.found_path == dir.file(name));
		CHECK(frame.controller.found_index == 1);
		CHECK(displayed(view).find("needle:with:colons") != std::string::npos);
	}
}

TEST_CASE("external search separates patterns and paths from options") {
	TestScreen screen;
	TestFrame frame;
	UI::Shell shell(frame.controller);
	TempDir dir;
	dir.write("-needle\n", "-file");
	WorkingDirectory cwd(dir.path);
	Search::View::exec({"-needle", "-file", ""}, shell);
	auto &view = TestSearch::current();
	finish(view, frame);
	CHECK(frame.status == "1 matches in 1 files (exit 0)");
	view.process(frame, Control::Return);
	CHECK(Path::same_file(frame.controller.found_path, dir.file("-file")));
}

TEST_CASE("search distinguishes no matches from invalid expressions and inaccessible paths") {
	TestScreen screen;
	TestFrame frame;
	UI::Shell shell(frame.controller);
	TempDir dir;
	dir.write("text\n");
	SUBCASE("no matches") {
		Search::View::exec({"absent", dir.path, ""}, shell);
		finish(TestSearch::current(), frame);
		CHECK(frame.status == "no matches (exit 1)");
		CHECK(frame.controller.searches == 1);
		CHECK_FALSE(frame.dialog);
	}
	SUBCASE("invalid regex") {
		Search::View::exec({"[", dir.path, ""}, shell);
		finish(TestSearch::current(), frame);
		CHECK(frame.status == "search failed: exit 2");
		CHECK(displayed(TestSearch::current()).find("grep:") != std::string::npos);
		CHECK(frame.controller.searches == 0);
	}
	SUBCASE("missing path") {
		Search::View::exec({"text", dir.file("missing"), ""}, shell);
		finish(TestSearch::current(), frame);
		CHECK(frame.status == "search failed: exit 2");
		CHECK(displayed(TestSearch::current()).find("missing") != std::string::npos);
	}
	SUBCASE("permission denied") {
		REQUIRE(chmod(dir.file().c_str(), 0000) == 0);
		Search::View::exec({"text", dir.file(), ""}, shell);
		finish(TestSearch::current(), frame);
		CHECK(frame.status == "search failed: exit 2");
		CHECK(displayed(TestSearch::current()).find("grep:") != std::string::npos);
		CHECK(chmod(dir.file().c_str(), 0600) == 0);
	}
}

TEST_CASE("search drains large stderr and retains an unterminated diagnostic") {
	TestScreen screen;
	TestFrame frame;
	UI::Shell shell(frame.controller);
	TempDir dir;
	FakeGrep grep(dir, "i=0; while [ $i -lt 10000 ]; do printf 'error message\\n' >&2; i=$((i+1)); done; printf 'last diagnostic' >&2; exit 2\n");
	Search::View::exec({"needle", dir.path, ""}, shell);
	auto &view = TestSearch::current();
	finish(view, frame);
	CHECK(frame.status == "search failed: exit 2");
	view.process(frame, Control::Return);
	CHECK(frame.controller.found_path.empty());
	for (unsigned i = 0; i < 1000; ++i) view.process(frame, KEY_NPAGE);
	CHECK(displayed(view).find("last diagnostic") != std::string::npos);
}

TEST_CASE("search reports malformed output without opening invalid records") {
	TestScreen screen;
	TestFrame frame;
	UI::Shell shell(frame.controller);
	TempDir dir;
	FakeGrep grep(dir, "printf 'file\\000bogus:match\\n'; exit 0\n");
	Search::View::exec({"needle", dir.path, ""}, shell);
	auto &view = TestSearch::current();
	finish(view, frame);
	CHECK(frame.status == "search failed: exit 0");
	CHECK(displayed(view).find("invalid search result") != std::string::npos);
	view.process(frame, Control::Return);
	CHECK(frame.controller.found_path.empty());
}

TEST_CASE("search cancellation retains partial matches and does not reopen the dialog") {
	TestScreen screen;
	TestFrame frame;
	UI::Shell shell(frame.controller);
	TempDir dir;
	FakeGrep grep(dir, "printf 'file\\0001:needle\\n'; exec sleep 30\n");
	Search::View::exec({"needle", dir.path, ""}, shell);
	auto &view = TestSearch::current();
	auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
	while (displayed(view).find("needle") == std::string::npos && std::chrono::steady_clock::now() < deadline) {
		view.poll(frame);
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	REQUIRE(displayed(view).find("needle") != std::string::npos);
	auto start = std::chrono::steady_clock::now();
	view.process(frame, Control::Kill);
	CHECK(std::chrono::steady_clock::now() - start < std::chrono::milliseconds(100));
	CHECK(frame.status == "cancelling");
	finish(view, frame);
	CHECK(frame.status == "cancelled");
	CHECK(frame.controller.searches == 0);
	view.process(frame, KEY_DOWN);
	view.process(frame, Control::Return);
	CHECK(frame.controller.found_index == 0);
	CHECK(frame.controller.found_path.find("file") != std::string::npos);
}

TEST_CASE("search results keep their launch directory when the application changes directory") {
	TestScreen screen;
	TestFrame frame;
	UI::Shell shell(frame.controller);
	TempDir original, later;
	original.write("needle\n");
	later.write("wrong\n");
	WorkingDirectory cwd(original.path);
	SUBCASE("directory changes during search") {
		Search::View::exec({"needle", ".", ""}, shell);
		REQUIRE(chdir(later.path.c_str()) == 0);
		finish(TestSearch::current(), frame);
	}
	SUBCASE("directory changes after search") {
		Search::View::exec({"needle", ".", ""}, shell);
		finish(TestSearch::current(), frame);
		REQUIRE(chdir(later.path.c_str()) == 0);
	}
	auto &view = TestSearch::current();
	CHECK(frame.status == "1 matches in 1 files (exit 0)");
	view.process(frame, KEY_DOWN);
	view.process(frame, Control::Return);
	CHECK(Path::same_file(frame.controller.found_path, original.file()));
	CHECK_FALSE(Path::same_file(frame.controller.found_path, later.file()));
}
