// ozette
// Copyright (C) 2014-2016 Mars J. Saxman
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

#include "app/syntax.h"
#include "ui/colors.h"
#include <map>
#include <sstream>

namespace Syntax {

const Rule cpreproc{"^#[A-Za-z]+", Token::Type::Keyword};
const Rule strdq{"\\\"([^\\\"]|(\\\\.))*\\\"", Token::Type::String};
const Rule strsq{"\\\'([^\\\']|(\\\\.))*\\\'", Token::Type::String};
const Rule cident{"[A-Za-z_][A-Za-z0-9_]*", Token::Type::Identifier};
const Rule cnumber{"(0([Xx][0-9A-Fa-f]+)?)|([1-9]+)", Token::Type::Literal};
const Rule slashcomment{"//(.*)$", Token::Type::Comment};
const Rule hashcomment{"#(.*)$", Token::Type::Comment};

const Grammar generic = {
};

const Grammar c = {
	Rule::keywords({
		"auto", "break", "case", "char", "const", "continue", "default", "do",
		"double", "else", "enum", "extern", "float", "for", "goto", "if",
		"inline", "int", "long", "register", "restrict", "return", "short",
		"signed", "sizeof", "static", "struct", "switch", "typedef", "union",
		"unsigned", "void", "volatile", "while", "_Alignas", "_Alignof",
		"_Atomic", "_Bool", "_Complex", "_Generic", "_Imaginary", "_Noreturn",
		"_Static_assert", "_Thread_local"
	}),
	cpreproc, strdq, strsq, cident, cnumber, slashcomment,
};

const Grammar cxx = {
	Rule::keywords({
		"alignas", "alignof", "and", "and_eq", "asm", "auto", "bitand",
		"bitor", "bool", "break", "case", "catch", "char", "char16_t",
		"char32_t", "class", "compl", "const", "constexpr", "const_cast",
		"continue", "decltype", "default", "delete", "do", "double",
		"dynamic_cast", "else", "enum", "explicit", "export", "extern",
		"false", "float", "for", "friend", "goto", "if", "inline", "int",
		"long", "mutable", "namespace", "new", "noexcept", "not", "not_eq",
		"nullptr", "operator", "or", "or_eq", "private", "protected", "public",
		"register", "reinterpret_cast", "return", "short", "signed", "sizeof",
		"static", "static_assert", "static_cast", "struct", "switch",
		"template", "this", "thread_local", "throw", "true", "try", "typedef",
		"typeid", "typename", "union", "unsigned", "using", "virtual", "void",
		"volatile", "wchar_t", "while", "xor", "xor_eq"
	}),
	cpreproc, strdq, strsq, cident, cnumber, slashcomment,
};

const Grammar ruby = {
	Rule::keywords({
		"alias", "and", "begin", "break", "case", "class", "def", "defined?",
		"do", "else", "elsif", "end", "ensure", "false", "for", "if", "in",
		"module", "next", "nil", "not", "or", "redo", "rescue", "retry",
		"return", "self", "super", "then", "true", "undef", "unless", "until",
		"when", "while", "yield"
	}),
	strdq, strsq,
	{"\\`([^\\']|(\\\\.))*\\`", Token::Type::String},
	{"[:]?[A-Za-z_][A-Za-z0-9_]*[?]?", Token::Type::Identifier},
	cnumber, hashcomment,
};

const Grammar make = {
	hashcomment,
};

const Grammar assembly = {
	{"\\.[A-Za-z0-9]+", Token::Type::Keyword},
	hashcomment,
};

const Grammar python = {
	Rule::keywords({
		"as", "assert", "break", "class", "continue", "def", "del", "elif",
		"else", "except", "exec", "finally", "for", "from", "global", "if",
		"import", "lambda", "pass", "print", "raise", "return", "try", "while",
		"with", "yield", "yield from",
	}),
	strdq, strsq, cident, cnumber, hashcomment,
};

const Grammar js = {
	Rule::keywords({
		"break", "case", "class", "catch", "const", "continue", "debugger",
		"default", "delete", "do", "else", "enum", "export", "extends",
		"finally", "for", "function", "if", "import", "in", "instanceof",
		"new", "return", "super", "switch", "this", "throw", "try", "typeof",
		"var", "void", "while", "with", "yield",
	}),
	strdq, strsq, cident, cnumber, slashcomment,
};

const std::map<std::string, const Grammar&> extensions = {
	{"c", c}, {"C", c},
	{"h", cxx}, {"H", cxx},
	{"cc", cxx}, {"cpp", cxx}, {"CPP", cxx}, {"c++", cxx}, {"cp", cxx},
	{"hh", cxx}, {"hpp", cxx}, {"HPP", cxx},
	{"rb", ruby},
	{"mk", make},
	{"s", assembly},
	{"py", python},
	{"js", js},
};
}

using namespace Syntax;

int Token::style() const {
	switch (type) {
		case Type::Identifier: return 0;
		case Type::Keyword: return UI::Colors::keyword();
		case Type::String: return UI::Colors::string();
		case Type::Literal: return UI::Colors::literal();
		case Type::Comment: return UI::Colors::comment();
		case Type::Error: return UI::Colors::error();
		default: return 0;
	}
}

Rule Rule::keywords(std::list<std::string> words) {
	std::stringstream buf;
	std::string delim;
	buf << "\\<(";
	for (auto &word: words) {
		buf << word << delim;
		delim = "|";
	}
	buf << ")\\>";
	return Rule(buf.str(), Token::Type::Keyword);
}

Tokens Syntax::parse(const Grammar &prods, const std::string &text) {
	Tokens out;
	for (size_t pos = 0; pos != std::string::npos;) {
		Token tk{std::string::npos, std::string::npos};
		for (auto &prod: prods) {
			auto match = prod.pattern.find(text, pos);
			if (match.empty()) continue;
			if (match.begin > tk.begin) continue;
			if (match.begin == tk.begin && match.end <= tk.end) continue;
			tk.begin = match.begin;
			tk.end = match.end;
			tk.type = prod.token;
		}
		if (tk.begin == tk.end) break;
		out.push_back(tk);
		pos = tk.end;
	}
	return out;
}

const Grammar &Syntax::lookup(const std::string &path) {
	std::string ext;
	size_t dotpos = path.find_last_of('.');
	if (dotpos != std::string::npos) {
		size_t slashpos = path.find_last_of('/');
		if (slashpos == std::string::npos || slashpos < dotpos) {
			ext = path.substr(dotpos+1);
		}
	}
	auto iter = extensions.find(ext);
	if (iter != extensions.end()) {
		return iter->second;
	}
	return generic;
}


