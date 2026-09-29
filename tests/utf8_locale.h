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

#ifndef TESTS_LOCALE_H
#define TESTS_LOCALE_H

#include <locale.h>
#include <stdexcept>
#include <string>

struct TestLocale {
	TestLocale(): previous(setlocale(LC_CTYPE, nullptr)) {
		if (!setlocale(LC_CTYPE, "C.UTF-8") && !setlocale(LC_CTYPE, "en_US.UTF-8")) {
			throw std::runtime_error("UTF-8 locale unavailable");
		}
	}
	~TestLocale() { setlocale(LC_CTYPE, previous.c_str()); }
	std::string previous;
};

#endif // TESTS_LOCALE_H
