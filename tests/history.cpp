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
#include <random>

namespace {
Editor::location_t location(const std::string &text, size_t offset) {
	Editor::location_t loc;
	for (size_t i = 0; i < offset; ++i) {
		if (text[i] == '\n') { ++loc.line; loc.offset = 0; }
		else ++loc.offset;
	}
	return loc;
}

std::string contents(Editor::Document &doc) {
	return doc.text(Editor::Range(doc.home(), doc.end()));
}

void check_cursor(Editor::Document &doc, Editor::location_t cursor) {
	REQUIRE(cursor.line <= doc.maxline());
	CHECK(cursor.offset <= doc.line(cursor.line).size());
}
} // namespace

TEST_CASE("bounded edit sequences undo and redo without losing bytes") {
	const std::string originals[] = {"", "alpha\nbeta", "a\r\nb\r\n",
		"a\r\nb\nc\r\nlast", "\n\r\n\n", "no final newline"};
	const std::string inserts[] = {"", "x", "ab", "\n", "p\nq\n",
		"\xc3\xa9", std::string("\0", 1)};
	// Fixed seeds make failures reproducible; every sequence stays small.
	for (unsigned seed = 0; seed < 48; ++seed) {
		CAPTURE(seed);
		std::mt19937 random(seed);
		TempDir dir;
		const auto &original = originals[seed % 6];
		dir.write(original);
		Editor::Document doc(dir.file());
		std::string model = contents(doc);
		const std::string initial = model;
		Editor::Update update;
		for (unsigned step = 0; step < 64; ++step) {
			CAPTURE(step);
			if (random() % 3 == 0) doc.commit();
			size_t a = random() % (model.size() + 1);
			size_t b = random() % (model.size() + 1);
			if (b < a) std::swap(a, b);
			Editor::Range span(location(model, a), location(model, b));
			const auto &text = inserts[random() % 7];
			switch (random() % 8) {
				case 0:
					doc.insert(span.begin(), text);
					model.insert(a, text);
					break;
				case 1:
					doc.erase(span);
					model.erase(a, b - a);
					break;
				case 2:
					doc.split(span.begin());
					model.insert(a, "\n");
					break;
				case 3: {
					Editor::Document::Edit edit(doc);
					doc.erase(span);
					doc.insert(span.begin(), text);
					model.replace(a, b - a, text);
					break;
				}
				case 4: {
					Editor::Document::Edit edit(doc);
					for (size_t line = 0; line <= doc.maxline(); ++line) {
						doc.insert(doc.home(line), std::string("  "));
					}
					for (size_t i = model.size(); i > 0; --i) {
						if (model[i - 1] == '\n') model.insert(i, "  ");
					}
					model.insert(0, "  ");
					break;
				}
				case 5: {
					std::string clip = model.substr(a, b - a);
					{
						Editor::Document::Edit cut(doc);
						doc.erase(span);
						model.erase(a, b - a);
					}
					size_t dest = random() % (model.size() + 1);
					Editor::Document::Edit paste(doc);
					doc.insert(location(model, dest), clip);
					model.insert(dest, clip);
					break;
				}
				case 6: {
					Editor::Document::Edit edit(doc);
					auto cursor = doc.split(span.begin());
					doc.insert(cursor, std::string("  "));
					model.insert(a, "\n  ");
					break;
				}
				case 7:
					// Contiguous characters exercise automatic typing groups.
					for (char ch: std::string("word")) {
						doc.insert(location(model, a), ch);
						model.insert(a++, 1, ch);
					}
					break;
			}
			REQUIRE(contents(doc) == model);
			if (step % 8 == 0 && doc.can_undo()) {
				check_cursor(doc, doc.undo(update));
				doc.commit();
				REQUIRE(doc.can_redo());
				check_cursor(doc, doc.redo(update));
				REQUIRE(contents(doc) == model);
			}
		}
		doc.Write(dir.file());
		const std::string edited = dir.read();
		for (unsigned cycle = 0; cycle < 2; ++cycle) {
			CAPTURE(cycle);
			std::vector<std::string> states;
			while (doc.can_undo()) {
				REQUIRE(states.size() < 128);
				states.push_back(contents(doc));
				check_cursor(doc, doc.undo(update));
			}
			CHECK(contents(doc) == initial);
			doc.Write(dir.file());
			CHECK(dir.read() == original);
			for (auto it = states.rbegin(); it != states.rend(); ++it) {
				REQUIRE(doc.can_redo());
				check_cursor(doc, doc.redo(update));
				REQUIRE(contents(doc) == *it);
			}
			CHECK_FALSE(doc.can_redo());
			doc.Write(dir.file());
			CHECK(dir.read() == edited);
		}
	}
}
