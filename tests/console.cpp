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
#include "console/console.h"
#include "ui.h"
#include <chrono>
#include <thread>

namespace {
class TestConsole : public Console::View {
public:
	static Console::View &current() { return *_instance; }
};

void finish(Console::View &view, TestFrame &frame) {
	auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
	do {
		view.poll(frame);
		if (frame.status != "running" && frame.status != "cancelling") return;
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	} while (std::chrono::steady_clock::now() < deadline);
	FAIL("console did not finish");
}
} // namespace

TEST_CASE("console displays command completion and preserves final output") {
	TestScreen screen;
	TestFrame frame;
	UI::Shell shell(frame.controller);
	Console::View::exec("command", "sh", {"-c", "printf 'final output'; printf 'last error' >&2; exit 7"}, shell);
	auto &view = TestConsole::current();
	finish(view, frame);
	CHECK(frame.title == "command");
	CHECK(frame.status == "exit 7");
	view.layout(0, 0, 6, 60);
	view.bring_forward();
	view.paint(UI::View::State::Focused);
	WINDOW *window = panel_window(panel_below(nullptr));
	REQUIRE(window);
	std::string displayed;
	for (int row = 0; row < 6; ++row) {
		char line[61] = {};
		REQUIRE(mvwinnstr(window, row, 0, line, 60) != ERR);
		displayed += line;
	}
	CHECK(displayed.find("final output") != std::string::npos);
	CHECK(displayed.find("last error") != std::string::npos);
	CHECK(displayed.find("[exit 7]") != std::string::npos);
}

TEST_CASE("console distinguishes failed launches from command exit statuses") {
	TestScreen screen;
	TestFrame frame;
	UI::Shell shell(frame.controller);
	Console::View::exec("missing command", "/no/such/ozette-command", {}, shell);
	finish(TestConsole::current(), frame);
	CHECK(frame.status.find("process error:") == 0);
}

TEST_CASE("console cancellation remains responsive and reports cancellation") {
	TestScreen screen;
	TestFrame frame;
	UI::Shell shell(frame.controller);
	Console::View::exec("command", "sh", {"-c", "exec sleep 30"}, shell);
	auto &view = TestConsole::current();
	auto start = std::chrono::steady_clock::now();
	view.process(frame, Control::Kill);
	CHECK(std::chrono::steady_clock::now() - start < std::chrono::milliseconds(100));
	CHECK(frame.status == "cancelling");
	finish(view, frame);
	CHECK(frame.status == "cancelled");
}
