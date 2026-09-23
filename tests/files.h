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

#ifndef TESTS_FILES_H
#define TESTS_FILES_H

#include <cstdio>
#include <cstdlib>
#include <dirent.h>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <unistd.h>

struct TempDir {
	TempDir() {
		char pattern[] = "/tmp/ozette-test-XXXXXX";
		char *created = mkdtemp(pattern);
		if (!created) throw std::runtime_error("mkdtemp failed");
		path = created;
	}
	~TempDir() {
		DIR *dir = opendir(path.c_str());
		if (dir) {
			while (auto entry = readdir(dir)) {
				std::string name = entry->d_name;
				if (name != "." && name != "..") std::remove(file(name).c_str());
			}
			closedir(dir);
		}
		rmdir(path.c_str());
	}
	TempDir(const TempDir &) = delete;
	TempDir &operator=(const TempDir &) = delete;
	std::string file(std::string name = "file") const { return path + "/" + name; }
	void write(std::string text, std::string name = "file") const {
		std::ofstream out;
		out.exceptions(std::ios::badbit | std::ios::failbit);
		out.open(file(name), std::ios::binary);
		out << text;
		out.close();
	}
	std::string read(std::string name = "file") const {
		std::ifstream in(file(name), std::ios::binary);
		if (!in) throw std::runtime_error("test file read failed");
		return std::string(std::istreambuf_iterator<char>(in), {});
	}
	std::string path;
};

#endif // TESTS_FILES_H
