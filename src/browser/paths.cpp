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

#include "browser/paths.h"
#include <dirent.h>
#include <sys/stat.h>
#include <cstring>
#include <assert.h>

static std::string complete_path(std::string path, bool only_dirs)
{
	// Figure out whether this string contains a directory path or just a name
	// fragment. If it's just a fragment, it is implicitly based in the current
	// working directory; otherwise, we'll search in the specified directory.
	size_t slashpos = path.find_last_of('/');
	bool found_slash = slashpos != std::string::npos;
	std::string base = found_slash? path.substr(0, slashpos + 1): ".";
	std::string fragment = found_slash? path.substr(slashpos + 1): path;
	if (fragment.empty()) return path;
	assert(!base.empty());
	size_t frag_len = fragment.size();
	// If the path begins with the magic home-dir marker, replace it with the
	// actual path to the home dir, because opendir won't parse it.
	if (base[0] == '~') {
		std::string homepath(getenv("HOME"));
		base = homepath + base.substr(1);
	}
	// Iterate through the items in this directory, looking for entries which
	// begin with the same chars as our name fragment.
	size_t matches = 0;
	std::string suffix;
	DIR *pdir = opendir(base.c_str());
	if (!pdir) return path;
	while (dirent *entry = readdir(pdir)) {
		// If we're only interested in directories, skip everything else.
		if (only_dirs && entry->d_type != DT_DIR) continue;
		// If the entry's name doesn't begin with our fragment, it can't be
		// a completion for this path.
		if (std::strncmp(entry->d_name, fragment.c_str(), frag_len)) continue;
		// We've found a match. If it's the first match, we'll use the rest of
		// its characters as our completion suffix. If it's a subsequent
		// match, we will use the leading sequence common to our existing
		// suffix and this file's name.
		std::string match(&entry->d_name[frag_len]);
		if (entry->d_type == DT_DIR) match.push_back('/');
		if (matches++) {
			// We've already found one match, so we will reduce the suffix
			// string to the characters common to this new entry's name.
			for (size_t i = 0; i < suffix.size(); ++i) {
				if (suffix[i] != match[i]) {
					suffix.resize(i);
					break;
				}
			}
		} else {
			// This was our first match, so we'll use the remainder of its
			// name as our completion suffix.
			suffix = match;
		}
	}
	closedir(pdir);
	return path + suffix;
}

std::string Browser::complete_file(std::string partial_path)
{
	return complete_path(partial_path, /*only_dirs*/false);
}

std::string Browser::complete_dir(std::string partial_path)
{
	return complete_path(partial_path, /*only_dirl*/true);
}


