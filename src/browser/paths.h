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

#ifndef BROWSER_PATHS_H
#define BROWSER_PATHS_H

#include <string>

namespace Browser {

// What is the full absolute path for the current working directory?
std::string current_dir();

// What is the full path to the user's home directory?
std::string home_dir();

// Given a path ending with a fragment of a name, find the characters which
// must follow that fragment and append them to the path. If the fragment
// constitutes a directory name, also append a slash, since we assume that
// the user will continue typing the name of a file within that directory.
std::string complete_file(std::string partial_path);

// Given a path ending with a fragment of a directory name, find the characters
// which must follow and append them to the path. File names will be ignored.
// We will not append a slash since we don't know whether this directory is
// the target or just another link in the ultimate chain.
std::string complete_dir(std::string partial_path);

// Reformat this path into its canonical absolute form. We will find the
// appropriate base directory, expand tildes and parent refs, strip any
// duplicate separators, and return a clean absolute path.
std::string absolute_path(std::string path);

// Format this path for compact user display.
std::string display_path(std::string path);

} // namespace Browser

#endif // BROWSER_PATHS_H

