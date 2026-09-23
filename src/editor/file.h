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

#ifndef EDITOR_FILE_H
#define EDITOR_FILE_H

#include <string>
#include <sys/stat.h>

namespace Editor {
class File {
public:
	std::string read(std::string path);
	bool exists() const { return _exists; }
private:
	std::string _path;
	struct stat _info = {};
	bool _exists = false;
};
} // namespace Editor

#endif // EDITOR_FILE_H
