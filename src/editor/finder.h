//
// ozette
// Copyright (C) 2015 Mars J. Saxman
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

#ifndef EDITOR_FINDER_H
#define EDITOR_FINDER_H

#include "ui/frame.h"
#include "editor/coordinates.h"

namespace Editor {
struct Finder {
	// What should the find dialog look for?
	std::string pattern;
	// Where should it begin looking?
	location_t anchor;
	// Function to select and display a matched string.
	typedef std::function<void(UI::Frame&, Range)> selector_t;
	selector_t selector = nullptr;
	// Function to search for a pattern and return a range of matches.
	typedef std::function<std::vector<Range>(std::string)> matcher_t;
	matcher_t matcher = nullptr;
	// Show the dialog and let the user perform a search.
	void show(UI::Frame&);
};
} // namespace Editor

#endif //EDITOR_FINDER_H

