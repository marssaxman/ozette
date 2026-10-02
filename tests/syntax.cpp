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
#include "app/syntax.h"
#include <vector>

namespace {
std::vector<std::string> keywords(const std::string &path, const std::string &text) {
	std::vector<std::string> out;
	for (const auto &token: Syntax::parse(Syntax::lookup(path), text)) {
		if (token.type == Syntax::Token::Type::Keyword) {
			out.push_back(text.substr(token.begin, token.end - token.begin));
		}
	}
	return out;
}
} // namespace

TEST_CASE("keyword rules match every supplied word independently") {
	const std::list<std::string> words = {
		"module", "return", "if", "in", "int", "while", "thread_local"
	};
	const auto rule = Syntax::Rule::keywords(words);
	for (const auto &word: words) {
		CAPTURE(word);
		const auto match = rule.pattern.find(word);
		REQUIRE_FALSE(match.empty());
		CHECK(match.begin == 0);
		CHECK(match.end == word.size());
		const auto surrounded = rule.pattern.find("(" + word + ");");
		CHECK(surrounded.begin == 1);
		CHECK(surrounded.end == word.size() + 1);
	}
}

TEST_CASE("keyword rules reject concatenations and identifier fragments") {
	const std::list<std::string> words = {"module", "return", "if", "in", "int"};
	const auto rule = Syntax::Rule::keywords(words);
	for (const auto &word: words) {
		CAPTURE(word);
		for (const auto &other: words) {
			CAPTURE(other);
			CHECK(rule.pattern.find(word + other).empty());
		}
		for (const auto &text: {"x" + word, "_" + word, word + "x",
			word + "_", word + "0"}) {
			CAPTURE(text);
			CHECK(rule.pattern.find(text).empty());
		}
	}
	CHECK(rule.pattern.find("unrelated").empty());
}

TEST_CASE("empty and single-word keyword lists remain usable") {
	const auto empty = Syntax::Rule::keywords({});
	CHECK(empty.pattern.find("").empty());
	CHECK(empty.pattern.find("module").empty());
	const auto single = Syntax::Rule::keywords({"module"});
	const auto match = single.pattern.find("module");
	CHECK(match.begin == 0);
	CHECK(match.end == 6);
	CHECK(single.pattern.find("modulemodule").empty());
}

TEST_CASE("language examples recognize keywords outside strings and comments") {
	struct Example {
		std::string path;
		std::string text;
		std::vector<std::string> expected;
	};
	const Example examples[] = {
		{"example.c", "auto int value; break; // auto break",
			{"auto", "int", "break"}},
		{"example.cpp", "alignas(8) int width = alignof(int); // alignas alignof",
			{"alignas", "int", "alignof", "int"}},
		{"example.rb", "alias old_name new_name; enabled = flag and true # alias and",
			{"alias", "and", "true"}},
		{"example.py", "import pkg as name; assert name != \"as assert\" # as assert",
			{"import", "as", "assert"}},
		{"example.py", "yield from source # yield from", {"yield from"}},
		{"example.js", "switch (name) { case \"break case\": break; } // break case",
			{"switch", "case", "break"}},
		{"example.proto", "optional int32 field = 1 [default = 1, deprecated = true]; // default deprecated",
			{"optional", "default", "deprecated", "true"}},
		{"example.go", "switch name { case \"break case\": break } // break case",
			{"switch", "case", "break"}},
		{"example.rs", "async fn task() { value.await as u32; } // abstract as",
			{"async", "fn", "await", "as"}},
		{"example.td", "assert true, \"assert bit\"; bit enabled = false; // assert bit",
			{"assert", "true", "bit", "false"}},
		{"Dockerfile", "ADD source destination # ADD ARG", {"ADD"}},
		{"Dockerfile", "ARG base=\"ADD ARG\" # ADD ARG", {"ARG"}},
		{"example.sh", "for item in values; do echo \"$item\"; done # case do",
			{"for", "in", "do", "done"}},
		{"example.mlir", "module { return } // module return", {"module", "return"}},
		{"example.mlir", "modulereturn", {}},
	};
	for (const auto &example: examples) {
		CAPTURE(example.path);
		CAPTURE(example.text);
		CHECK(keywords(example.path, example.text) == example.expected);
	}
}
