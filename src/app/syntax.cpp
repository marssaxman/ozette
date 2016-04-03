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

namespace Syntax {
Grammar generic = {};
Grammar c = {
	{"\\<("
		"auto|break|case|char|const|continue|default|do|double|else|enum"
		"|extern|float|for|goto|if|inline|int|long|register|restrict|return"
		"|short|signed|sizeof|static|struct|switch|typedef|union|unsigned"
		"|void|volatile|while|_Alignas|_Alignof|_Atomic|_Bool|_Complex"
		"|_Generic|_Imaginary|_Noreturn|_Static_assert|_Thread_local"
	")\\>", Token::Type::Keyword},
	{"\\\"([^\\\"]|(\\\\.))*\\\"", Token::Type::String},
	{"\\'([^\\']|(\\\\.))*\\'", Token::Type::String},
	{"//(.*)$", Token::Type::Comment},
};
Grammar cxx = {
	{"\\<("
		"alignas|alignof|and|and_eq|asm|auto|bitand|bitor|bool|break|case"
		"|catch|char|char16_t|char32_t|class|compl|const|constexpr|const_cast"
		"|continue|decltype|default|delete|do|double|dynamic_cast|else|enum"
		"|explicit|export|extern|false|float|for|friend|goto|if|inline|int"
		"|long|mutable|namespace|new|noexcept|not|not_eq|nullptr|operator|or"
		"|or_eq|private|protected|public|register|reinterpret_cast|return"
		"|short|signed|sizeof|static|static_assert|static_cast|struct|switch"
		"|template|this|thread_local|throw|true|try|typedef|typeid|typename"
		"|union|unsigned|using|virtual|void|volatile|wchar_t|while|xor|xor_eq"
	")\\>", Token::Type::Keyword},
	{"\\\"([^\\\"]|(\\\\.))*\\\"", Token::Type::String},
	{"\\'([^\\']|(\\\\.))*\\'", Token::Type::String},
	{"//(.*)$", Token::Type::Comment},
};
Grammar ruby = {
	{"\\<("
		"do|begin|end|undef|alias|if|while|unless|until|return|yield|and|or"
		"|not|super|defined?|elsif|else|case|when|rescue|ensure|class|module"
		"|def|then|nil|self|true|false"
	")\\>", Token::Type::Keyword},
	{"\\\"([^\\\"]|(\\\\.))*\\\"", Token::Type::String},
	{"\\'([^\\']|(\\\\.))*\\'", Token::Type::String},
	{"\\`([^\\']|(\\\\.))*\\`", Token::Type::String},
	{"#(.*)$", Token::Type::Comment},
};
Grammar make = {
	{"#(.*)$", Token::Type::Comment},
};
Grammar assembly = {
	{"\\.[A-Za-z0-9]+", Token::Type::Keyword},
	{"#(.*)$", Token::Type::Comment},
};
Grammar python = {
	{"\\<("
		"as|assert|break|class|continue|def|del|elif|else|except|exec|finally"
		"|for|from|global|if|import|lambda|pass|print|raise|return|try|while"
		"|with|yield|yield from"
	")\\>", Token::Type::Keyword},
	{"\\\"([^\\\"]|(\\\\.))*\\\"", Token::Type::String},
	{"\\'([^\\']|(\\\\.))*\\'", Token::Type::String},
	{"#(.*)$", Token::Type::Comment},
};
Grammar js = {
	{"\\<("
		"break|case|class|catch|const|continue|debugger|default|delete|do|else"
		"|enum|export|extends|finally|for|function|if|import|in|instanceof|new"
		"|return|super|switch|this|throw|try|typeof|var|void|while|with|yield"
	")\\>", Token::Type::Keyword},
	{"\\\"([^\\\"]|(\\\\.))*\\\"", Token::Type::String},
	{"\\'([^\\']|(\\\\.))*\\'", Token::Type::String},
	{"//(.*)$", Token::Type::Comment},
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

Regex::Regex(std::string pattern):
	_pattern(pattern) {
	compile();
}

Regex::Regex(const Regex &other):
	_pattern(other._pattern) {
	compile();
}

Regex &Regex::operator=(const Regex &other) {
	regfree(&_re);
	_pattern = other._pattern;
	compile();
	return *this;
}

Regex::~Regex() {
	regfree(&_re);
}

void Regex::compile() {
	_comp_err = regcomp(&_re, _pattern.c_str(), REG_EXTENDED);
}

Regex::Match Regex::find(const std::string &text, size_t offset) const {
	regmatch_t rm;
	Match out;
	if (_comp_err) return out;
	if (regexec(&_re, text.c_str() + offset, 1, &rm, 0)) return out;
	out.begin = rm.rm_so + offset;
	out.end = rm.rm_eo + offset;
	return out;
}

Regex::Matches Regex::find_all(const std::string &text) const {
	Match found = find(text);
	std::list<Match> out;
	while (!found.empty()) {
		out.push_back(found);
		found = find(text, found.end);
	}
	return out;
}

int Token::style() const {
	switch (type) {
		case Type::Keyword: return UI::Colors::keyword();
		case Type::String: return UI::Colors::string();
		case Type::Comment: return UI::Colors::comment();
		default: return 0;
	}
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


