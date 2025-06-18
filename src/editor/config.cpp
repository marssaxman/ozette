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

#include <algorithm>
#include "app/path.h"
#include "editor/config.h"
#include "editor/ec_fnmatch.h"
#include <fstream>
#include <map>
#include <stack>
#include <string>
#include <vector>

namespace {
struct section {
	std::string pattern;
	std::map<std::string, std::string> definitions;
};
struct configfile {
	std::string path;
	std::map<std::string, std::string> definitions;
	std::vector<section> sections;
	void parse(std::string line) {
		// Strip off leading whitespace, if any.
		while (!line.empty() && isspace(line.front())) {
			line.erase(0, 1);
		}
		// Skip blank lines and comment lines.
		if (line.empty() || 0 == line.find_first_of("#;")) {
			return;
		}
		// If the line begins with '[', it must contain a matching ']',
		// and the text between is a filename glob expression beginning
		// a section of property assignments specific to those files.
		if ('[' == line.front()) {
			size_t endpos = line.find_first_of(']');
			if (endpos != std::string::npos) {
				sections.emplace_back();
				sections.back().pattern = line.substr(1, endpos-1);
			} else {
				error = true;
			}
			return;
		}
		// If the line begins with any other character, it must be a
		// name[:=]value style property assignment, followed optionally by
		// a comment. Whitespace on either side of the separator character
		// will be ignored.
		size_t seppos = line.find_first_of(":=");
		if (seppos != std::string::npos) {
			// Transform the whole thing to lowercase, because the spec calls
			// for case-insensitive key & value comparisons
			std::transform(line.begin(), line.end(), line.begin(), ::tolower);
			std::string key = line.substr(0, seppos);
			std::string val = line.substr(seppos+1);
			// Trim trailing whitespace from the key
			while (!key.empty() && isspace(key.back())) {
				key.pop_back();
			}
			// Trim leading whitespace from the value
			while (!val.empty() && isspace(val.front())) {
				val.erase(0, 1);
			}
			// Trim trailing comment from the value, if any
			size_t commentpos = val.find_first_of(";#");
			if (commentpos != std::string::npos) {
				val.resize(commentpos);
			}
			if (key.empty()) {
				error = true;
				return;
			}
			// an empty value appears to be legal, or at least common
			if (sections.empty()) {
				definitions[key] = val;
			} else {
				sections.back().definitions[key] = val;
			}
		} else {
			error = true;
		}
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
	while (slashpos > 0 && slashpos != std::string::npos && !root) {
		// Truncate the path to locate the containing directory.
		path = path.substr(0, slashpos);
		slashpos = path.find_last_of('/');
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
		if (!data.error) {
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
			auto const &pattern = sec.pattern;
			if (pattern.find_first_of('/') != std::string::npos) {
				fnflag |= EC_FNM_PATHNAME;
			}
			if (0 == ec_fnmatch(pattern.c_str(), rel_path.c_str(), fnflag)) {
				for (auto &pair: sec.definitions) {
					apply(pair.first, pair.second);
				}
			}
		}
	}
}

void Editor::Config::reset() {
	// Default values for all settings, to be overridden by values specified
	// in .editorconfig files as we may discover them.
	_indent_style = TAB;
	_indent_size = 4;
	// We don't actually use the rest of these settings, but we'll keep track
	// of them because they are defined in the specification.
	_tab_width = 4;
	_end_of_line = LF;
	_charset = UTF8;
	_trim_trailing_whitespace = true;
	_insert_final_newline = true;
	_max_line_length = 80;
}

void Editor::Config::apply(std::string key, std::string val) {
	if (key == "indent_style") {
		if (val == "tab") _indent_style = TAB;
		else if (val == "space") _indent_style = SPACE;
	} else if (key == "indent_size") {
		_indent_size = std::stoul(val, 0, 10);
	} else if (key == "tab_width") {
		_tab_width = std::stoul(val, 0, 10);
	} else if (key == "end_of_line") {
		if (val == "cr") _end_of_line = CR;
		else if (val == "lf") _end_of_line = LF;
		else if (val == "crlf") _end_of_line = CRLF;
	} else if (key == "charset") {
		if (val == "utf-8") _charset = UTF8;
		else if (val == "latin1") _charset = LATIN1;
		else if (val == "utf-16be") _charset = UTF16BE;
		else if (val == "utf-16le") _charset = UTF16LE;
		else if (val == "utf-8-bom") _charset = UTF8BOM;
	} else if (key == "trim_trailing_whitespace") {
		if (val == "true") _trim_trailing_whitespace = true;
		else if (val == "false") _trim_trailing_whitespace = false;
	} else if (key == "insert_final_newline") {
		if (val == "true") _insert_final_newline = true;
		else if (val == "false") _insert_final_newline = false;
	} else if (key == "max_line_length") {
		_max_line_length = std::stoul(val, 0, 10);
	}
}

