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
#include "app/control.h"
#include "files.h"
#include <functional>
#include <signal.h>
#include <sys/wait.h>

namespace {
class TestApp : public Ozette {
public:
	void cache_read(std::string, std::vector<std::string> &lines) override { lines.clear(); }
	void cache_write(std::string, const std::vector<std::string> &) override {}
	std::string get_clipboard() override {
		if (wait_for_build) {
			for (int i = 0; i < 200 && access("built-z", F_OK) != 0; ++i) usleep(10000);
		}
		return "";
	}
	bool wait_for_build = false;
};

void run_app(const TempDir &dir, std::vector<int> keys,
		std::function<void(TestApp &)> prepare) {
	fflush(nullptr);
	pid_t child = fork();
	REQUIRE(child >= 0);
	if (child == 0) {
		alarm(10);
		signal(SIGIO, SIG_IGN);
		signal(SIGPIPE, SIG_IGN);
		if (!freopen("/dev/null", "w", stdout) || !freopen("/dev/null", "r", stdin)) _exit(2);
		if (chdir(dir.path.c_str()) || setenv("TERM", "xterm", 1)) _exit(2);
		try {
			TestApp app;
			prepare(app);
			keys.push_back(Control::Escape);
			for (auto key = keys.rbegin(); key != keys.rend(); ++key) {
				if (ungetch(*key) == ERR) _exit(3);
			}
			app.run();
		} catch (...) { _exit(4); }
		_exit(0);
	}
	int status;
	REQUIRE(waitpid(child, &status, 0) == child);
	REQUIRE(WIFEXITED(status));
	REQUIRE(WEXITSTATUS(status) == 0);
}

void enter_path(std::vector<int> &keys, int command, std::string path) {
	keys.push_back(command);
	for (char ch: path) keys.push_back(ch);
	keys.push_back(Control::Return);
}

void save_copy(std::vector<int> &keys) {
	enter_path(keys, Control::SaveAs, "copy");
}
} // namespace

TEST_CASE("Build stops when a save fails") {
	TempDir dir;
	dir.write("old");
	REQUIRE(link(dir.file().c_str(), dir.file("alias").c_str()) == 0);
	std::vector<int> keys = {'x', KEY_F(5), 'y'};
	save_copy(keys);
	keys.push_back(Control::Quit);
	run_app(dir, keys, [&](TestApp &app) { app.edit_file(dir.file()); });
	CHECK(dir.read() == "old");
	CHECK(dir.read("alias") == "old");
	CHECK(dir.read("copy") == "xyold");
}

TEST_CASE("Build focuses the editor with a conflict and allows cancellation") {
	TempDir dir;
	dir.write("old", "a");
	dir.write("other", "z");
	std::vector<int> keys = {Control::LeftArrow, 'x', Control::RightArrow,
		KEY_F(5), Control::Escape, 'y'};
	save_copy(keys);
	keys.push_back(Control::Quit);
	run_app(dir, keys, [&](TestApp &app) {
		app.edit_file(dir.file("a"));
		app.edit_file(dir.file("z"));
		dir.write("external", "a");
	});
	CHECK(dir.read("a") == "external");
	CHECK(dir.read("z") == "other");
	CHECK(dir.read("copy") == "xyold");
}

TEST_CASE("Quit stops at a failed save and keeps the remaining editors open") {
	TempDir dir;
	dir.write("old", "a");
	dir.write("other", "z");
	REQUIRE(link(dir.file("a").c_str(), dir.file("alias").c_str()) == 0);
	std::vector<int> keys = {'z', Control::LeftArrow, 'x', Control::RightArrow,
		Control::Quit, 'y', 'y'};
	save_copy(keys);
	keys.push_back(Control::Quit);
	keys.push_back('n');
	run_app(dir, keys, [&](TestApp &app) {
		app.edit_file(dir.file("a"));
		app.edit_file(dir.file("z"));
	});
	CHECK(dir.read("a") == "old");
	CHECK(dir.read("z") == "other");
	CHECK(dir.read("copy") == "xyold");
}

TEST_CASE("Quit allows a save conflict to be cancelled in its own window") {
	TempDir dir;
	dir.write("old", "a");
	dir.write("other", "z");
	std::vector<int> keys = {Control::LeftArrow, 'x', Control::RightArrow,
		Control::Quit, 'y', Control::Escape, 'y'};
	save_copy(keys);
	keys.push_back(Control::Quit);
	run_app(dir, keys, [&](TestApp &app) {
		app.edit_file(dir.file("a"));
		app.edit_file(dir.file("z"));
		dir.write("external", "a");
	});
	CHECK(dir.read("a") == "external");
	CHECK(dir.read("z") == "other");
	CHECK(dir.read("copy") == "xyold");
}

TEST_CASE("Quit saves all modified files before closing") {
	TempDir dir;
	dir.write("old", "a");
	dir.write("other", "z");
	run_app(dir, {'z', Control::LeftArrow, 'x', Control::RightArrow,
		Control::Quit, 'y'}, [&](TestApp &app) {
		app.edit_file(dir.file("a"));
		app.edit_file(dir.file("z"));
	});
	CHECK(dir.read("a") == "xold");
	CHECK(dir.read("z") == "zother");
}

TEST_CASE("Build sees the saved contents of every editor") {
	TempDir dir;
	dir.write("old", "a");
	dir.write("other", "z");
	dir.write("all:\n\tcp a built-a\n\tcp z built-z\n", "Makefile");
	run_app(dir, {'z', Control::LeftArrow, 'x', Control::RightArrow,
		KEY_F(5), Control::RightArrow, Control::Paste, Control::Quit}, [&](TestApp &app) {
		app.edit_file(dir.file("a"));
		app.edit_file(dir.file("z"));
		app.wait_for_build = true;
	});
	CHECK(dir.read("built-a") == "xold");
	CHECK(dir.read("built-z") == "zother");
}

TEST_CASE("an initial read error leaves the application usable") {
	TempDir dir;
	REQUIRE(mkfifo(dir.file("pipe").c_str(), 0600) == 0);
	dir.write("old");
	run_app(dir, {'x', Control::Quit, 'y'}, [&](TestApp &app) {
		app.edit_file(dir.file("pipe"));
		app.edit_file(dir.file());
	});
	CHECK(dir.read() == "xold");
}

TEST_CASE("separate deletions survive undo through the command loop") {
	TempDir dir;
	dir.write("abcdefgh");
	run_app(dir, {KEY_DC, KEY_RIGHT, KEY_RIGHT, KEY_DC,
		Control::Undo, Control::Undo, 'x', Control::Quit, 'y'},
		[&](TestApp &app) { app.edit_file(dir.file()); });
	CHECK(dir.read() == "axbcdefgh");
}

TEST_CASE("switching editor tabs after undo preserves redo") {
	TempDir dir;
	dir.write("old", "a");
	dir.write("other", "z");
	run_app(dir, {'x', Control::Undo, Control::LeftArrow,
		Control::RightArrow, Control::Redo, Control::Quit, 'y'},
		[&](TestApp &app) {
			app.edit_file(dir.file("a"));
			app.edit_file(dir.file("z"));
		});
	CHECK(dir.read("a") == "old");
	CHECK(dir.read("z") == "xother");
}

TEST_CASE("Open and New reuse an editor with unsaved changes") {
	TempDir dir;
	dir.write("old");
	int command = Control::Open;
	SUBCASE("Open") { command = Control::Open; }
	SUBCASE("New") { command = Control::NewFile; }
	std::vector<int> keys = {'x'};
	enter_path(keys, command, "./file");
	keys.push_back('y');
	enter_path(keys, command, "file");
	keys.insert(keys.end(), {'z', Control::Close, 'y'});
	run_app(dir, keys, [&](TestApp &app) { app.edit_file("file"); });
	CHECK(dir.read() == "xyzold");
}

TEST_CASE("Open and New share missing targets through directory aliases") {
	TempDir dir;
	REQUIRE(symlink(".", dir.file("directory").c_str()) == 0);
	std::vector<int> keys = {'x'};
	enter_path(keys, Control::NewFile, "directory/file");
	keys.push_back('y');
	enter_path(keys, Control::Open, "file");
	keys.insert(keys.end(), {'z', Control::Quit, 'y'});
	run_app(dir, keys, [&](TestApp &app) { app.edit_file("./file"); });
	CHECK(dir.read() == "xyz");
}

TEST_CASE("opening a file alias reuses the original editor") {
	TempDir dir;
	dir.write("old");
	SUBCASE("symbolic link") {
		REQUIRE(symlink("file", dir.file("alias").c_str()) == 0);
	}
	SUBCASE("hard link") {
		REQUIRE(link(dir.file().c_str(), dir.file("alias").c_str()) == 0);
	}
	std::vector<int> keys = {'x'};
	enter_path(keys, Control::Open, "alias");
	keys.push_back('y');
	save_copy(keys);
	keys.push_back(Control::Close);
	run_app(dir, keys, [&](TestApp &app) { app.edit_file("file"); });
	CHECK(dir.read() == "old");
	CHECK(dir.read("copy") == "xyold");
}

TEST_CASE("relative Save As survives directory changes and reopening") {
	TempDir dir;
	dir.write("old");
	REQUIRE(mkdir(dir.file("sub").c_str(), 0700) == 0);
	std::vector<int> keys = {'x'};
	enter_path(keys, Control::SaveAs, "renamed");
	enter_path(keys, Control::Directory, "sub");
	keys.insert(keys.end(), {'y', Control::Save});
	enter_path(keys, Control::Open, "../renamed");
	keys.insert(keys.end(), {'z', Control::Close, 'y'});
	run_app(dir, keys, [&](TestApp &app) { app.edit_file("file"); });
	CHECK(dir.read() == "old");
	CHECK(dir.read("renamed") == "xyzold");
	CHECK(access(dir.file("sub/renamed").c_str(), F_OK) != 0);
}

TEST_CASE("Save As releases the old target for a separate editor") {
	TempDir dir;
	dir.write("old");
	std::vector<int> keys = {'x'};
	save_copy(keys);
	enter_path(keys, Control::Open, "file");
	keys.insert(keys.end(), {'y', Control::Quit, 'y'});
	run_app(dir, keys, [&](TestApp &app) { app.edit_file("file"); });
	CHECK(dir.read() == "yold");
	CHECK(dir.read("copy") == "xold");
}

TEST_CASE("Save As refuses another editor's target") {
	TempDir dir;
	dir.write("old");
	bool existing = true, modified = true;
	SUBCASE("modified file") {}
	SUBCASE("unmodified file") { modified = false; }
	SUBCASE("new file") { existing = false; }
	if (existing) dir.write("other", "other");
	std::vector<int> keys;
	if (modified) keys.push_back('y');
	keys.insert(keys.end(), {Control::LeftArrow, 'x'});
	enter_path(keys, Control::SaveAs, "other");
	keys.insert(keys.end(), {'z', Control::Close, 'y', 'w', Control::Close, 'y'});
	run_app(dir, keys, [&](TestApp &app) {
		app.edit_file("file");
		app.edit_file("other");
	});
	CHECK(dir.read() == "xzold");
	CHECK(dir.read("other") == (modified? "yw": "w") + std::string(existing? "other": ""));
}

TEST_CASE("Save As detects open destinations through aliases") {
	TempDir dir;
	dir.write("old");
	dir.write("other", "other");
	SUBCASE("symbolic link") {
		REQUIRE(symlink("other", dir.file("alias").c_str()) == 0);
	}
	SUBCASE("hard link") {
		REQUIRE(link(dir.file("other").c_str(), dir.file("alias").c_str()) == 0);
	}
	std::vector<int> keys = {'y', Control::LeftArrow, 'x'};
	enter_path(keys, Control::SaveAs, "alias");
	keys.insert(keys.end(), {'z', Control::Close, 'y'});
	save_copy(keys);
	keys.push_back(Control::Close);
	run_app(dir, keys, [&](TestApp &app) {
		app.edit_file("file");
		app.edit_file("other");
	});
	CHECK(dir.read() == "xzold");
	CHECK(dir.read("other") == "other");
	CHECK(dir.read("alias") == "other");
	CHECK(dir.read("copy") == "yother");
}

TEST_CASE("Save As detects missing open destinations through directory aliases") {
	TempDir dir;
	dir.write("old");
	REQUIRE(symlink(".", dir.file("directory").c_str()) == 0);
	std::vector<int> keys = {'y', Control::LeftArrow, 'x'};
	enter_path(keys, Control::SaveAs, "directory/new");
	keys.insert(keys.end(), {'z', Control::Quit, 'y'});
	run_app(dir, keys, [&](TestApp &app) {
		app.edit_file("file");
		app.edit_file("new");
	});
	CHECK(dir.read() == "xzold");
	CHECK(dir.read("new") == "y");
}

TEST_CASE("Save As rechecks the registry after overwrite confirmation") {
	TempDir dir;
	dir.write("old");
	dir.write("other", "other");
	std::vector<int> keys = {'x'};
	enter_path(keys, Control::SaveAs, "other");
	keys.push_back(Control::UpArrow);
	enter_path(keys, Control::Open, "other");
	keys.insert(keys.end(), {'y', Control::LeftArrow, 'y', 'z',
		Control::Close, 'y', Control::Close, 'y', Control::Quit});
	run_app(dir, keys, [&](TestApp &app) { app.edit_file("file"); });
	CHECK(dir.read() == "xzold");
	CHECK(dir.read("other") == "yother");
}

TEST_CASE("cancelled and failed Save As leave the original editor registered") {
	TempDir dir;
	dir.write("old");
	dir.write("other", "other");
	std::vector<int> keys = {'x'};
	SUBCASE("cancelled overwrite") {
		enter_path(keys, Control::SaveAs, "other");
		keys.push_back(Control::Escape);
	}
	SUBCASE("failed write") { enter_path(keys, Control::SaveAs, "missing/copy"); }
	enter_path(keys, Control::Open, "file");
	keys.insert(keys.end(), {'y', Control::Close, 'y'});
	run_app(dir, keys, [&](TestApp &app) { app.edit_file("file"); });
	CHECK(dir.read() == "xyold");
	CHECK(dir.read("other") == "other");
}

TEST_CASE("Save As can adopt the editor's own symlink alias") {
	TempDir dir;
	dir.write("old");
	REQUIRE(symlink("file", dir.file("alias").c_str()) == 0);
	std::vector<int> keys = {'x'};
	enter_path(keys, Control::SaveAs, "alias");
	keys.push_back('y');
	enter_path(keys, Control::Open, "file");
	keys.insert(keys.end(), {'z', Control::Close, 'y'});
	run_app(dir, keys, [&](TestApp &app) { app.edit_file("file"); });
	CHECK(dir.read() == "xzold");
	CHECK(dir.read("alias") == "xzold");
	struct stat info;
	REQUIRE(lstat(dir.file("alias").c_str(), &info) == 0);
	CHECK(S_ISLNK(info.st_mode));
}

TEST_CASE("an atomic save keeps the editor reachable through file aliases") {
	TempDir dir;
	dir.write("old");
	REQUIRE(symlink("file", dir.file("alias").c_str()) == 0);
	std::vector<int> keys = {'x', Control::Save};
	enter_path(keys, Control::Open, "alias");
	keys.insert(keys.end(), {'y', Control::Quit, 'y'});
	run_app(dir, keys, [&](TestApp &app) { app.edit_file("file"); });
	CHECK(dir.read() == "xyold");
}

TEST_CASE("closing remains possible after the target disappears") {
	TempDir dir;
	dir.write("old");
	run_app(dir, {'x', Control::Close, 'n'}, [&](TestApp &app) {
		app.edit_file("file");
		if (unlink(dir.file().c_str())) throw std::runtime_error("unlink failed");
	});
	CHECK(access(dir.file().c_str(), F_OK) != 0);
}

TEST_CASE("a pending Quit does not discard edits in a reopened editor") {
	TempDir dir;
	dir.write("old", "a");
	dir.write("other", "b");
	std::vector<int> keys = {'y', Control::LeftArrow, 'x', Control::Quit,
		Control::RightArrow, Control::Close, 'n'};
	keys.push_back(Control::UpArrow);
	enter_path(keys, Control::Open, "b");
	keys.insert(keys.end(), {'z', Control::LeftArrow, 'n', 'y', Control::Quit});
	run_app(dir, keys, [&](TestApp &app) {
		app.edit_file("a");
		app.edit_file("b");
	});
	CHECK(dir.read("a") == "old");
	CHECK(dir.read("b") == "zother");
}

TEST_CASE("saving rechecks aliases changed after opening") {
	TempDir dir;
	dir.write("old");
	dir.write("other", "other");
	std::vector<int> keys = {'y', Control::LeftArrow, 'x', Control::Save};
	save_copy(keys);
	keys.insert(keys.end(), {Control::Close, Control::Close, 'n'});
	run_app(dir, keys, [&](TestApp &app) {
		app.edit_file("file");
		app.edit_file("other");
		if (unlink(dir.file("other").c_str()) ||
				symlink("file", dir.file("other").c_str())) {
			throw std::runtime_error("symlink failed");
		}
	});
	CHECK(dir.read() == "old");
	CHECK(dir.read("other") == "old");
	CHECK(dir.read("copy") == "xold");
}
