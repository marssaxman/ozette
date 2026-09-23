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

#include "editor/file.h"
#include "app/path.h"
#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <fcntl.h>
#include <unistd.h>

namespace {
class Descriptor {
public:
	explicit Descriptor(int fd): fd(fd) {}
	~Descriptor() { if (fd >= 0) ::close(fd); }
	Descriptor(const Descriptor &) = delete;
	Descriptor &operator=(const Descriptor &) = delete;
	int fd;
};

std::runtime_error io_error(std::string action, std::string path) {
	int code = errno;
	return std::runtime_error(action + " " + path + ": " + std::strerror(code));
}
} // namespace

std::string Editor::File::read(std::string path) {
	path = Path::absolute(path);
	Descriptor file(open(path.c_str(), O_RDONLY | O_NONBLOCK | O_CLOEXEC));
	if (file.fd < 0) {
		if (errno != ENOENT) throw io_error("Can't read", path);
		struct stat link;
		if (lstat(path.c_str(), &link) == 0) {
			throw std::runtime_error("Missing link target: " + path);
		}
		if (errno != ENOENT) throw io_error("Can't read", path);
		_path = path;
		_exists = false;
		return "";
	}
	struct stat info;
	if (fstat(file.fd, &info)) throw io_error("Can't read", path);
	if (!S_ISREG(info.st_mode)) {
		throw std::runtime_error("Not a regular file: " + path);
	}
	std::string text;
	char buf[8192];
	for (;;) {
		ssize_t count = ::read(file.fd, buf, sizeof(buf));
		if (count == 0) break;
		if (count < 0) {
			if (errno == EINTR) continue;
			throw io_error("Can't read", path);
		}
		text.append(buf, count);
	}
	_path = path;
	_info = info;
	_exists = true;
	return text;
}
