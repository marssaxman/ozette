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

#include "ui/helpbar.h"

UI::HelpBar::Label::Label(std::initializer_list<std::string> items) {
	auto iter = items.begin();
	if (iter != items.end()) {
		std::string m = *iter++;
		if (m.size() < 2) {
			m.insert(0, 2 - m.size(), ' ');
		}
		mnemonic[0] = m[0];
		mnemonic[1] = m[1];
	}
	if (iter != items.end()) {
		text = *iter;
	}
}

