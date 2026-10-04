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
#include "app/ozette.h"
#include "files.h"
#include <functional>
#include <signal.h>
#include <sys/wait.h>

namespace {
void run_state(const TempDir &dir, const char *state_home,
		std::function<void(Ozette &)> action) {
	fflush(nullptr);
	pid_t child = fork();
	REQUIRE(child >= 0);
	if (child == 0) {
		alarm(10);
		if (!freopen("/dev/null", "w", stdout) || !freopen("/dev/null", "r", stdin)) _exit(2);
		if (chdir(dir.path.c_str()) || setenv("TERM", "xterm", 1) ||
				setenv("HOME", dir.path.c_str(), 1)) _exit(2);
		if (state_home? setenv("XDG_STATE_HOME", state_home, 1): unsetenv("XDG_STATE_HOME")) _exit(2);
		try {
			Ozette app;
			action(app);
		} catch (...) { _exit(3); }
		_exit(0);
	}
	int status;
	REQUIRE(waitpid(child, &status, 0) == child);
	REQUIRE(WIFEXITED(status));
	REQUIRE(WEXITSTATUS(status) == 0);
}

void record_state(Ozette &app, const TempDir &dir, std::string name) {
	std::vector<std::string> lines = {"stale"};
	app.state_read(name, lines);
	std::string text;
	for (auto &line: lines) text += line + "\n";
	dir.write(text, "read-" + name);
}

mode_t permissions(std::string path) {
	struct stat st;
	REQUIRE(stat(path.c_str(), &st) == 0);
	return st.st_mode & 0777;
}
} // namespace

TEST_CASE("application state creates the default directory hierarchy") {
	TempDir dir;
	std::vector<std::string> names = {StateKey::kSessionState,
		StateKey::kExpansionState, StateKey::kSearchSpec};
	run_state(dir, nullptr, [&](Ozette &app) {
		for (auto &name: names) {
			app.state_write(name, {"first", "", "last"});
			record_state(app, dir, name);
		}
	});
	for (auto &name: names) {
		CHECK(dir.read(".local/state/ozette/" + name) == "first\n\nlast\n");
		CHECK(dir.read("read-" + name) == "first\n\nlast\n");
	}
	CHECK(permissions(dir.file(".local")) == 0700);
	CHECK(permissions(dir.file(".local/state")) == 0700);
	CHECK(permissions(dir.file(".local/state/ozette")) == 0700);
	CHECK(access(dir.file(".cache").c_str(), F_OK) != 0);
}

TEST_CASE("a custom state base keeps application files in the ozette subdirectory") {
	TempDir dir;
	std::string state = dir.file("custom/nested/state");
	SUBCASE("ordinary path") {}
	SUBCASE("trailing and repeated separators") { state = dir.file("custom//nested/state/"); }
	run_state(dir, state.c_str(), [&](Ozette &app) {
		app.state_write(StateKey::kSearchSpec, {"needle", "haystack", "*.cpp"});
		record_state(app, dir, StateKey::kSearchSpec);
	});
	CHECK(dir.read("custom/nested/state/ozette/search_spec") == "needle\nhaystack\n*.cpp\n");
	CHECK(dir.read("read-search_spec") == "needle\nhaystack\n*.cpp\n");
	CHECK(access(dir.file("custom/nested/state/search_spec").c_str(), F_OK) != 0);
	CHECK(access(dir.file(".local").c_str(), F_OK) != 0);
}

TEST_CASE("empty and relative state bases use the default across directory changes") {
	TempDir dir;
	const char *state = "";
	SUBCASE("empty") {}
	SUBCASE("relative") { state = "relative-state"; }
	SUBCASE("tilde") { state = "~/state"; }
	REQUIRE(mkdir(dir.file("other").c_str(), 0700) == 0);
	run_state(dir, state, [&](Ozette &app) {
		app.state_write(StateKey::kSearchSpec, {"first"});
		app.change_dir(dir.file("other"));
		record_state(app, dir, StateKey::kSearchSpec);
		app.state_write(StateKey::kSessionState, {"second"});
	});
	CHECK(dir.read("read-search_spec") == "first\n");
	CHECK(dir.read(".local/state/ozette/search_spec") == "first\n");
	CHECK(dir.read(".local/state/ozette/open_editors") == "second\n");
	CHECK(access(dir.file("relative-state").c_str(), F_OK) != 0);
	CHECK(access(dir.file("other/relative-state").c_str(), F_OK) != 0);
	CHECK(access(dir.file("~").c_str(), F_OK) != 0);
	CHECK(access(dir.file("other/~").c_str(), F_OK) != 0);
}

TEST_CASE("state directory creation preserves existing directory permissions") {
	TempDir dir;
	std::string state = dir.file("state");
	REQUIRE(mkdir(state.c_str(), 0755) == 0);
	REQUIRE(chmod(state.c_str(), 0755) == 0);
	dir.write("other application", "state/search_spec");
	run_state(dir, state.c_str(), [&](Ozette &app) {
		app.state_write(StateKey::kSearchSpec, {"ozette"});
	});
	CHECK(permissions(state) == 0755);
	CHECK(permissions(state + "/ozette") == 0700);
	CHECK(dir.read("state/search_spec") == "other application");
	REQUIRE(chmod((state + "/ozette").c_str(), 0750) == 0);
	run_state(dir, state.c_str(), [&](Ozette &app) {
		app.state_write(StateKey::kSearchSpec, {"updated"});
	});
	CHECK(permissions(state + "/ozette") == 0750);
	CHECK(dir.read("state/ozette/search_spec") == "updated\n");
}

TEST_CASE("unavailable state directories leave persistence recoverable") {
	TempDir dir;
	std::string state = dir.file("blocked/nested");
	SUBCASE("parent is a file") { dir.write("keep", "blocked"); }
	SUBCASE("parent is unwritable") {
		if (geteuid() == 0) return;
		REQUIRE(mkdir(dir.file("blocked").c_str(), 0500) == 0);
	}
	run_state(dir, state.c_str(), [&](Ozette &app) {
		app.state_write(StateKey::kSearchSpec, {"saved"});
		record_state(app, dir, StateKey::kSearchSpec);
	});
	CHECK(dir.read("read-search_spec").empty());
	CHECK(access((state + "/ozette/search_spec").c_str(), F_OK) != 0);
	struct stat st;
	REQUIRE(stat(dir.file("blocked").c_str(), &st) == 0);
	if (S_ISDIR(st.st_mode)) {
		REQUIRE(chmod(dir.file("blocked").c_str(), 0700) == 0);
	} else {
		CHECK(dir.read("blocked") == "keep");
	}
}

TEST_CASE("reading missing state ignores old cache files and creates no directories") {
	TempDir dir;
	REQUIRE(mkdir(dir.file(".cache").c_str(), 0700) == 0);
	REQUIRE(mkdir(dir.file(".cache/ozette").c_str(), 0700) == 0);
	dir.write("old search\n", ".cache/ozette/search_spec");
	run_state(dir, nullptr, [&](Ozette &app) {
		record_state(app, dir, StateKey::kSearchSpec);
	});
	CHECK(dir.read("read-search_spec").empty());
	CHECK(access(dir.file(".local").c_str(), F_OK) != 0);
	CHECK(dir.read(".cache/ozette/search_spec") == "old search\n");
}
