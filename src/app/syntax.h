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

#ifndef APP_SYNTAX_H
#define APP_SYNTAX_H

#include "app/regex.h"
#include <string>
#include <list>

namespace Syntax {

struct Token {
	size_t begin;
	size_t end;
	enum class Type {
		Identifier,
		Keyword,
		String,
		Literal,
		Comment,
		Symbol,
		Error,
	} type;
	int style() const;
};
typedef std::list<Token> Tokens;

struct Rule {
	Rule(const char *p, Token::Type t): pattern(std::string(p)), token(t) {}
	Rule(const std::string &p, Token::Type t): pattern(p), token(t) {}
	static Rule keywords(std::list<std::string>);
	Regex pattern;
	Token::Type token;
};
typedef std::list<Rule> Grammar;

Tokens parse(const Grammar&, const std::string&);
const Grammar &lookup(const std::string &path);

} // namespace Syntax

#endif //APP_SYNTAX_H

