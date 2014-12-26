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
	_identifier("\\<[\\w]+\\>"),
	_keyword("\\<(const|using|namespace|return|if|while|do|for|auto|unsigned|"
		"switch|case|default)\\>"),
	_string("\\\"(.*)[^\\\\]\\\""),
	_comment("//(.*)$"),
	_trailing_space("[[:space:]]+$")
{
}

Grammar::Tokens Grammar::parse(const std::string &text) const
{
	Tokens out;
	auto spaces = tokenize(
			_trailing_space.find_all(text), Token::Type::TrailingSpace);
	out.insert(out.end(), spaces.begin(), spaces.end());

	auto comments = tokenize(_comment.find_all(text), Token::Type::Comment);
	out.insert(out.end(), comments.begin(), comments.end());

	auto strs = tokenize(_string.find_all(text), Token::Type::String);
	out.insert(out.end(), strs.begin(), strs.end());

	auto keys = tokenize(_keyword.find_all(text), Token::Type::Keyword);
	out.insert(out.end(), keys.begin(), keys.end());

	auto idents = tokenize(_identifier.find_all(text), Token::Type::Identifier);
	out.insert(out.end(), idents.begin(), idents.end());
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
