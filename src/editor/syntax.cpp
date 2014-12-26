//
// lindi
// Copyright (C) 2014 Mars J. Saxman
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
//

#include "syntax.h"
#include "colors.h"

using namespace Editor;

Regex::Regex(std::string pattern)
{
	_comp_err = regcomp(&_re, pattern.c_str(), REG_EXTENDED);
}

Regex::~Regex()
{
	regfree(&_re);
}

Regex::Match Regex::find(const std::string &text, size_t offset) const
{
	regmatch_t rm;
	Match out;
	if (_comp_err) return out;
	if (regexec(&_re, text.c_str() + offset, 1, &rm, 0)) return out;
	out.begin = rm.rm_so + offset;
	out.end = rm.rm_eo + offset;
	return out;
}

Regex::Matches Regex::find_all(const std::string &text) const
{
	Match found = find(text);
	std::list<Match> out;
	while (!found.empty()) {
		out.push_back(found);
		found = find(text, found.end);
	}
	return out;
}

Grammar::Grammar():
	_identifier("\\<\\w+\\>"),
	_keyword("\\<("
		"void|bool|int|short|long|signed|unsigned|char|float|double|const"
		"|enum|struct|class|union|typedef|extern|static|inline|register"
		"|public|protected|private|this|friend|virtual|mutable|volatile"
		"|template|typename|using|namespace|try|throw|catch|operator"
		"|if|while|do|for|else|switch|case|default"
		"|new|delete|goto|continue|break|return"
		")\\>"),
	_string("\\\"(.*)[^\\\\]\\\""),
	_comment("//(.*)$"),
	_trailing_space("[[:space:]]+$")
{
}

Grammar::Tokens Grammar::parse(const std::string &text) const
{
	struct prod_t {
		Token::Type type;
		const Regex &re;
	};
	const size_t kProdcount = 5;
	prod_t prods[kProdcount] = {
		{Token::Type::Keyword, _keyword},
		{Token::Type::Identifier, _identifier},
		{Token::Type::String, _string},
		{Token::Type::Comment, _comment},
		{Token::Type::TrailingSpace, _trailing_space}
	};
	Tokens out;
	size_t pos = 0;
	while (pos != std::string::npos) {
		Regex::Match found_pos;
		Token::Type found_type;
		for (unsigned i = 0; i < kProdcount; ++i) {
			auto &prod = prods[i];
			auto match = prod.re.find(text, pos);
			if (match.empty()) continue;
			if (found_pos.empty() || match.begin < found_pos.begin) {
				found_pos = match;
				found_type = prod.type;
			}
		}
		if (!found_pos.empty()) {
			Token tk = {found_pos.begin, found_pos.end, found_type};
			out.push_back(tk);
		}
		pos = found_pos.end;
	}
	return out;
}

Grammar::Tokens Grammar::tokenize(
		const Regex::Matches &matches, Token::Type type) const
{
	Tokens out;
	for (auto &match: matches) {
		Token tk = {match.begin, match.end, type};
		out.push_back(tk);
	}
	return out;
}

int Grammar::Token::style() const
{
	switch (type) {
		case Token::Type::Identifier: return UI::Colors::identifier();
		case Token::Type::Keyword: return UI::Colors::keyword();
		case Token::Type::String: return UI::Colors::string();
		case Token::Type::Comment: return UI::Colors::comment();
		case Token::Type::TrailingSpace: return UI::Colors::trailing_space();
		default: return 0;
	}
}
