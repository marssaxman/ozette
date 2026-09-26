// ozette
// Copyright (C) 2015-2016 Mars J. Saxman
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

#include "app/path.h"
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <cstring>
#include <cstdlib>
#include <assert.h>

namespace {
std::string resolved(std::string path) {
	char *name = realpath(path.c_str(), nullptr);
	if (!name) return "";
	std::string out(name);
	free(name);
	return out;
}

std::string target(std::string path) {
	// New files have no inode. Resolve as much of their parent path as exists.
	std::string suffix;
	for (;;) {
		std::string base = resolved(path);
		if (!base.empty()) return (base == "/" && !suffix.empty()? "": base) + suffix;
		size_t slash = path.find_last_of('/');
		if (slash == std::string::npos || path == "/") return path + suffix;
		suffix = path.substr(slash) + suffix;
		path.resize(slash? slash: 1);
	}
}
} // namespace

std::string Path::home_dir() {
	return std::string(getenv("HOME"));
}

std::string Path::current_dir() {
	std::string out;
	char *cwd = getcwd(NULL, 0);
	if (cwd) {
		out = std::string(cwd);
		free(cwd);
	}
	return out;
}

static std::string complete_path(std::string path, bool only_dirs) {
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
		base = Path::home_dir() + base.substr(1);
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

std::string Path::complete_file(std::string partial_path) {
	return complete_path(partial_path, /*only_dirs*/false);
}

std::string Path::complete_dir(std::string partial_path) {
	return complete_path(partial_path, /*only_dirs*/true);
}

std::string Path::absolute(std::string path) {
	// Expand this path to a full path relative to the filesystem root.
	if (path.empty()) {
		return current_dir();
	}
	std::string out;
	size_t offset = 0;
	if (path[0] == '/') {
		offset = 1;
	} else if (path == "~" || path.substr(0, 2) == "~/") {
		offset = 1;
		out = home_dir();
	} else {
		out = current_dir();
	}
	if (out == "/") out.clear();
	while (offset != std::string::npos) {
		size_t segpos = path.find_first_of('/', offset);
		std::string seg;
		if (segpos == std::string::npos) {
			seg = path.substr(offset);
			offset = segpos;
		} else {
			seg = path.substr(offset, segpos - offset);
			offset = segpos + 1;
		}
		if (seg.empty()) continue;
		if (seg == ".") continue;
		if (seg == "..") {
			std::string parent = resolved(out + "/..");
			if (parent.empty()) out += "/..";
			else out = parent == "/"? "": parent;
			continue;
		}
		out += "/" + seg;
	}
	return out.empty()? "/": out;
}

bool Path::same_file(std::string a, std::string b) {
	a = absolute(a);
	b = absolute(b);
	if (a == b) return true;
	struct stat first, second;
	if (stat(a.c_str(), &first) == 0 && stat(b.c_str(), &second) == 0 &&
			first.st_dev == second.st_dev && first.st_ino == second.st_ino) return true;
	return target(a) == target(b);
}

std::string Path::display(std::string path) {
	std::string cwd = current_dir();
	size_t cwdsize = cwd.size();
	if (cwd == "/" && path.size() > 1 && path[0] == '/') return path.substr(1);
	if (path.size() > cwdsize && path.substr(0, cwdsize) == cwd && path[cwdsize] == '/') {
		return path.substr(cwdsize + 1);
	}
	std::string home = home_dir();
	size_t homesize = home.size();
	if (path == home || (path.size() > homesize &&
			path.substr(0, homesize) == home && path[homesize] == '/')) {
		return "~" + path.substr(homesize);
	}
	return path;

}
