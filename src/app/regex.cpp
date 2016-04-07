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

#include "app/regex.h"

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

