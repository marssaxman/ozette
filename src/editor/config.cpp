// ozette
// Copyright (C) 2016 Mars J. Saxman
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
#include "editor/config.h"
#include "editor/ec_fnmatch.h"
#include <fstream>
#include <map>
#include <stack>
#include <string>
#include <vector>

namespace {
std::string trim(std::string text) {
	const char *space = " \t\r\n\v\f";
	size_t first = text.find_first_not_of(space);
	if (first == std::string::npos) return "";
	return text.substr(first, text.find_last_not_of(space) - first + 1);
}

std::string lowercase(std::string text) {
	for (char &ch: text) {
		if (ch >= 'A' && ch <= 'Z') ch += 'a' - 'A';
	}
	return text;
}

bool width(const std::string &text, unsigned &out) {
	// Bound both arithmetic and the work done by an indentation command.
	// Unsupported values leave the caller's previous/default setting alone.
	const unsigned maximum = 256;
	unsigned value = 0;
	if (text.empty()) return false;
	for (char ch: text) {
		if (ch < '0' || ch > '9') return false;
		unsigned digit = ch - '0';
		if (value > (maximum - digit) / 10) return false;
		value = value * 10 + digit;
	}
	if (value == 0) return false;
	out = value;
	return true;
}

void apply(std::map<std::string, std::string> &settings,
		const std::string &key, const std::string &value) {
	if (key != "indent_style" && key != "indent_size" && key != "tab_width") return;
	if (value == "unset") {
		settings.erase(key);
		return;
	}
	unsigned size;
	bool valid = key == "indent_style"? (value == "tab" || value == "space"):
		((key == "indent_size" && value == "tab") || width(value, size));
	if (valid) settings[key] = value;
}

struct section {
	std::string pattern;
	std::map<std::string, std::string> definitions;
};
struct configfile {
	std::string path;
	std::map<std::string, std::string> definitions;
	std::vector<section> sections;
	void parse(std::string line) {
		line = trim(line);
		if (line.empty() || line.front() == '#' || line.front() == ';') return;
		if (line.front() == '[') {
			if (line.size() < 3 || line.back() != ']') {
				error = true;
				return;
			}
			sections.emplace_back();
			sections.back().pattern = line.substr(1, line.size() - 2);
			return;
		}
		size_t separator = line.find('=');
		if (separator == std::string::npos) {
			error = true;
			return;
		}
		std::string key = lowercase(trim(line.substr(0, separator)));
		std::string value = lowercase(trim(line.substr(separator + 1)));
		if (key.empty()) {
			error = true;
			return;
		}
		// Inline # and ; characters belong to the value, not to a comment.
		if (sections.empty()) definitions[key] = value;
		else sections.back().definitions[key] = value;
	}
	bool error = false;
};
} // namespace

void Editor::Config::load(std::string file_path) {
	reset();
	// Load appropriate config settings for the file at this path.
	// Beginning in its directory, we will descend toward the root, looking
	// for files named ".editorconfig". Each time we find such a file, we will
	// parse it and store the contents. If a file contains a global attribute
	// named "root" which is equal to "true", we will stop searching.
	file_path = Path::absolute(file_path);
	std::stack<configfile> files;
	std::string path = file_path;
	size_t slashpos = path.find_last_of('/');
	bool root = false;
	while (slashpos != std::string::npos && !root) {
		// Truncate the path to locate the containing directory.
		path = path.substr(0, slashpos);
		slashpos = path.empty()? std::string::npos: path.find_last_of('/');
		// Look for a file named ".editorconfig" here.
		std::ifstream infile(path + "/.editorconfig");
		if (!infile) continue;
		// An editorconfig file must be UTF-8 encoded, and its lines may end
		// with either CRLF or LF. Parse each line one by one.
		configfile data;
		data.path = path;
		for (std::string line; std::getline(infile, line);) {
			data.parse(line);
		}
		// If the parse succeeded, check to see if this file contained a root
		// definition, then add it to our config file stack.
		if (!data.error && !infile.bad()) {
			auto rootiter = data.definitions.find("root");
			if (rootiter != data.definitions.end()) {
				root = rootiter->second == "true";
			}
			files.push(std::move(data));
		}
	}
	// Now that we have collected the list of applicable config files, iterate
	// from the last (rootmost) file found back to the first (leafmost).
	// For each file, iterate through the sections, comparing each section name
	// to the target file path as a glob expression. For each section with a
	// matching glob, apply its property definitions to the current config,
	// overriding any previous definitions which may have existed.
	std::map<std::string, std::string> settings;
	while (!files.empty()) {
		configfile current = std::move(files.top());
		files.pop();
		// Globs are evaluated relative to the config file's directory, so
		// chop off the relevant piece of the absolute path to get a relative
		// path for the target file.
		std::string rel_path = file_path.substr(current.path.size() + 1);
		for (auto &sec: current.sections) {
			// If the file's path matches this section's glob pattern, apply
			// the section's definitions to the current configuration.
			int fnflag = 0;
			std::string pattern = sec.pattern;
			if (pattern.find_first_of('/') != std::string::npos) {
				fnflag |= EC_FNM_PATHNAME;
			}
			if (!pattern.empty() && pattern.front() == '/') pattern.erase(0, 1);
			if (0 == ec_fnmatch(pattern.c_str(), rel_path.c_str(), fnflag)) {
				for (auto &pair: sec.definitions) {
					apply(settings, pair.first, pair.second);
				}
			}
		}
	}
	// Resolve symbolic indentation only after all parent and child settings
	// have been merged; a nearer tab_width can change an inherited "tab".
	if (settings["indent_style"] == "space") _indent_style = SPACE;
	const auto &size = settings["indent_size"];
	if (size == "tab" || (size.empty() && _indent_style == TAB)) {
		width(settings["tab_width"], _indent_size);
	} else {
		width(size, _indent_size);
	}
}

void Editor::Config::reset() {
	_indent_style = TAB;
	_indent_size = 4;
}
