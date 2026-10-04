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

#ifndef TESTS_UI_H
#define TESTS_UI_H

#include <cstdio>
#include <stdexcept>
#include "ui/view.h"

struct TestScreen {
	TestScreen() {
		input = tmpfile();
		output = tmpfile();
		if (!input || !output) throw std::runtime_error("tmpfile failed");
		screen = newterm("xterm", output, input);
		if (!screen) throw std::runtime_error("newterm failed");
	}
	~TestScreen() {
		endwin();
		delscreen(screen);
		fclose(output);
		fclose(input);
	}
	FILE *input;
	FILE *output;
	SCREEN *screen;
};

struct TestController : Controller {
	void change_dir(std::string) override {}
	void edit_file(std::string) override {}
	bool can_save_file(const Editor::View &, std::string) override { return allow_save; }
	void close_file(Editor::View &view) override { closed = &view; }
	void find_in_file(std::string path, size_t index) override {
		found_path = path;
		found_index = index;
	}
	void begin_search() override { ++searches; }
	void search_for(Search::spec) override {}
	void set_clipboard(std::string text) override { clipboard = text; }
	std::string get_clipboard() override { return clipboard; }
	void state_read(std::string, std::vector<std::string> &) override {}
	void state_write(std::string, const std::vector<std::string> &) override {}
	Editor::View *closed = nullptr;
	bool allow_save = true;
	std::string clipboard;
	std::string found_path;
	size_t found_index = 0;
	unsigned searches = 0;
};

struct TestFrame : UI::Frame {
	Controller &app() override { return controller; }
	void repaint() override {}
	void set_title(std::string text) override { title = text; }
	void set_status(std::string text) override { status = text; }
	void show_result(std::string text) override { result = text; }
	void show_dialog(std::unique_ptr<UI::View> &&view) override {
		dialog = std::move(view);
	}
	void answer(int ch) {
		if (!dialog) throw std::runtime_error("no dialog");
		auto current = std::move(dialog);
		if (current->process(*this, ch)) dialog = std::move(current);
	}
	void enter(std::string text) {
		for (unsigned char ch: text) answer(ch);
		answer(Control::Return);
	}
	TestController controller;
	std::unique_ptr<UI::View> dialog;
	std::string title, status, result;
};

#endif // TESTS_UI_H
