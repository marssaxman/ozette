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
	std::function<void(UI::Frame&, Range)> selector;
	// The user has taken some action which commits this pattern as their most
	// recent search string.
	std::function<void(std::string)> committer;
	// The result of a search: an iterator through a list of matches.
	class Matches {
	public:
		virtual ~Matches() {}
		// Never return an empty match set. Return nullptr instead.
		// How many matches were found?
		virtual size_t size() const = 0;
		// What is the location of the current match?
		virtual Range value() const = 0;
		// What is the (zero-based) index of the current match?
		virtual size_t index() const = 0;
		// Advance to the next match in the list.
		virtual void next() = 0;
	};
	// Function to search for a pattern and return a range of matches.
	std::function<std::unique_ptr<Matches>(std::string, location_t)> matcher;
	// Show the dialog and let the user perform a search.
	void show(UI::Frame&);
};
} // namespace Editor

#endif //EDITOR_FINDER_H

